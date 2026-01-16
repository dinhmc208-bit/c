#include "scan_engine.h"
#include "net_tools.h"
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>
#include <iomanip>

ScanEngine::ScanEngine() : running(false) {
}

ScanEngine::~ScanEngine() {
    stop();
    if (ips_file.is_open()) {
        ips_file.close();
    }
}

bool ScanEngine::init(const std::string& range, int port, double timeout, int max_threads) {
    if (!NetTools::parseRange(range, &ip_range)) {
        return false;
    }
    
    scan_port = port;
    scan_timeout = timeout;
    
    thread_pool = std::make_unique<ThreadPool>(max_threads);
    io_multiplexer = std::make_unique<IOMultiplexer>(100000);
    
    ips_file.open("output/ips.txt", std::ios::app);
    if (!ips_file.is_open()) {
        return false;
    }
    
    // Calculate total
    if (!ip_range.is_ipv6) {
        total = ip_range.v4.end - ip_range.v4.start + 1;
    } else {
        // IPv6: simplified count
        total = 1000000; // Placeholder
    }
    
    current = 0;
    found = 0;
    running = true;
    
    return true;
}

void ScanEngine::start() {
    if (!running) {
        return;
    }
    
    // Start output thread
    std::thread output(&ScanEngine::outputThread, this);
    output.detach();
    
    // Generate IPs and scan
    if (!ip_range.is_ipv6) {
        // IPv4: iterate through range
        for (uint32_t ip_int = ip_range.v4.start; ip_int <= ip_range.v4.end && running; ip_int++) {
            std::string ip = NetTools::intToIPv4(ip_int);
            
            thread_pool->enqueue([this, ip]() {
                scanWorker(ip);
            });
            
            current++;
        }
    } else {
        // IPv6: simplified - scan specific IPs
        std::vector<std::string> ips = NetTools::generateIPs(ip_range, 10000);
        for (const auto& ip : ips) {
            if (!running) break;
            
            thread_pool->enqueue([this, ip]() {
                scanWorker(ip);
            });
            
            current++;
        }
    }
    
    // Wait for all tasks to complete
    thread_pool->waitAll();
    running = false;
}

void ScanEngine::stop() {
    running = false;
}

void ScanEngine::scanWorker(const std::string& ip) {
    if (connectAndCheck(ip, scan_port, scan_timeout)) {
        writeResult(ip, scan_port);
        found++;
    }
}

bool ScanEngine::connectAndCheck(const std::string& ip, int port, double timeout) {
    struct sockaddr_storage addr;
    socklen_t addrlen;
    int sockfd;
    bool is_ipv6 = NetTools::getIPVersion(ip) == 6;
    
    if (!NetTools::parseIP(ip, &addr)) {
        return false;
    }
    
    if (is_ipv6) {
        reinterpret_cast<struct sockaddr_in6*>(&addr)->sin6_port = htons(port);
        addrlen = sizeof(struct sockaddr_in6);
        sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    } else {
        reinterpret_cast<struct sockaddr_in*>(&addr)->sin_port = htons(port);
        addrlen = sizeof(struct sockaddr_in);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
    }
    
    if (sockfd < 0) {
        return false;
    }
    
    // Set non-blocking
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
    // Try connect
    int result = ::connect(sockfd, reinterpret_cast<struct sockaddr*>(&addr), addrlen);
    
    if (result < 0 && errno != EINPROGRESS) {
        close(sockfd);
        return false;
    }
    
    // Set blocking for timeout
    fcntl(sockfd, F_SETFL, flags);
    
    // Set timeout
    struct timeval tv;
    tv.tv_sec = static_cast<int>(timeout);
    tv.tv_usec = static_cast<int>((timeout - tv.tv_sec) * 1000000);
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    // Check connection
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(sockfd, &write_fds);
    
    struct timeval timeout_tv = tv;
    result = select(sockfd + 1, nullptr, &write_fds, nullptr, &timeout_tv);
    
    if (result <= 0) {
        close(sockfd);
        return false;
    }
    
    // Check for errors
    int error = 0;
    socklen_t error_len = sizeof(error);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &error_len) < 0 || error != 0) {
        close(sockfd);
        return false;
    }
    
    // Connection successful
    close(sockfd);
    return true;
}

void ScanEngine::writeResult(const std::string& ip, int port) {
    std::lock_guard<std::mutex> lock(file_mutex);
    bool is_ipv6 = NetTools::getIPVersion(ip) == 6;
    
    if (is_ipv6) {
        ips_file << "[" << ip << "]:" << port << "\n";
    } else {
        ips_file << ip << ":" << port << "\n";
    }
    ips_file.flush();
}

void ScanEngine::outputThread() {
    while (running || current < total) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        uint64_t cur = current.load();
        uint64_t tot = total.load();
        uint64_t fnd = found.load();
        
        std::cout << "\r Current [" << cur << "/" << tot << "] Found: " << fnd << "   " << std::flush;
        
        if (cur >= tot && !running) {
            break;
        }
    }
    std::cout << "\n";
}

