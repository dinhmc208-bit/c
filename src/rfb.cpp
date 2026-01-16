#include "rfb.h"
#include "des.h"
#include "net_tools.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <netdb.h>

RFBProtocol::RFBProtocol(const std::string& host, const std::string& password, 
                        int port, double timeout)
    : host(host), password(password), port(port), timeout(timeout), 
      sockfd(-1), is_ipv6(false) {
    result.connected = false;
    result.is_rfb = false;
    result.null_auth = false;
}

RFBProtocol::~RFBProtocol() {
    close();
}

bool RFBProtocol::createSocket() {
    struct sockaddr_storage addr;
    socklen_t addrlen;
    
    if (!NetTools::parseIP(host, &addr)) {
        // Try DNS resolution
        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        
        if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0) {
            return false;
        }
        
        memcpy(&addr, res->ai_addr, res->ai_addrlen);
        addrlen = res->ai_addrlen;
        is_ipv6 = (res->ai_family == AF_INET6);
        freeaddrinfo(res);
    } else {
        is_ipv6 = (addr.ss_family == AF_INET6);
        if (is_ipv6) {
            reinterpret_cast<struct sockaddr_in6*>(&addr)->sin6_port = htons(port);
            addrlen = sizeof(struct sockaddr_in6);
        } else {
            reinterpret_cast<struct sockaddr_in*>(&addr)->sin_port = htons(port);
            addrlen = sizeof(struct sockaddr_in);
        }
    }
    
    sockfd = socket(is_ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return false;
    }
    
    // Set timeout
    struct timeval tv;
    tv.tv_sec = static_cast<int>(timeout);
    tv.tv_usec = static_cast<int>((timeout - tv.tv_sec) * 1000000);
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    // Connect
    if (::connect(sockfd, reinterpret_cast<struct sockaddr*>(&addr), addrlen) < 0) {
        close();
        return false;
    }
    
    return true;
}

bool RFBProtocol::connect() {
    if (!createSocket()) {
        return false;
    }
    
    if (!connInit()) {
        return false;
    }
    
    if (!clientAuth()) {
        return false;
    }
    
    return result.connected;
}

bool RFBProtocol::connInit() {
    char buffer[12];
    if (!receiveData(buffer, 12)) {
        return false;
    }
    
    if (memcmp(buffer, "RFB", 3) != 0) {
        return false;
    }
    
    result.is_rfb = true;
    
    const char* response = "RFB 003.003\n";
    return sendData(response, strlen(response));
}

bool RFBProtocol::clientAuth() {
    uint32_t method;
    if (!receiveData(&method, 4)) {
        return false;
    }
    
    method = ntohl(method);
    
    if (method == 0) {
        // Connection failed
        uint32_t length;
        if (!receiveData(&length, 4)) {
            return false;
        }
        length = ntohl(length);
        
        std::vector<char> msg(length);
        if (!receiveData(msg.data(), length)) {
            return false;
        }
        result.error_message = std::string(msg.data(), length);
        return false;
    } else if (method == 1) {
        // None authentication
        result.null_auth = true;
        return clientInit();
    } else if (method == 2) {
        // VNC authentication
        return vncAuth();
    }
    
    return false;
}

bool RFBProtocol::vncAuth() {
    uint8_t challenge[16];
    if (!receiveData(challenge, 16)) {
        return false;
    }
    
    if (!sendPassword(challenge)) {
        return false;
    }
    
    uint32_t result_code;
    if (!receiveData(&result_code, 4)) {
        return false;
    }
    
    result_code = ntohl(result_code);
    
    if (result_code == 0) {
        return clientInit();
    } else {
        result.error_message = "WRONG PASSWORD";
        return false;
    }
}

bool RFBProtocol::clientInit() {
    uint8_t shared = 1;
    if (!sendData(&shared, 1)) {
        return false;
    }
    
    char buffer[24];
    if (!receiveData(buffer, 24)) {
        return false;
    }
    
    uint16_t width, height;
    uint32_t namelen;
    memcpy(&width, buffer, 2);
    memcpy(&height, buffer + 2, 2);
    memcpy(&namelen, buffer + 20, 4);
    
    width = ntohs(width);
    height = ntohs(height);
    namelen = ntohl(namelen);
    
    if (namelen > 0 && namelen < 256) {
        std::vector<char> name(namelen);
        if (receiveData(name.data(), namelen)) {
            result.server_name = std::string(name.data(), namelen);
        }
    }
    
    result.connected = true;
    return true;
}

bool RFBProtocol::sendPassword(const uint8_t* challenge) {
    std::vector<uint8_t> response = DES::vnc_encrypt(password, challenge);
    return sendData(response.data(), response.size());
}

bool RFBProtocol::receiveData(void* buffer, size_t len) {
    size_t received = 0;
    while (received < len) {
        ssize_t n = recv(sockfd, static_cast<char*>(buffer) + received, len - received, 0);
        if (n <= 0) {
            return false;
        }
        received += n;
    }
    return true;
}

bool RFBProtocol::sendData(const void* buffer, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(sockfd, static_cast<const char*>(buffer) + sent, len - sent, 0);
        if (n <= 0) {
            return false;
        }
        sent += n;
    }
    return true;
}

void RFBProtocol::close() {
    if (sockfd >= 0) {
        ::close(sockfd);
        sockfd = -1;
    }
}

