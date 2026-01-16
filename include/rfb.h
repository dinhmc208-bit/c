#pragma once

#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory>

struct RFBResult {
    bool connected;
    bool is_rfb;
    bool null_auth;
    std::string server_name;
    std::string error_message;
};

class RFBProtocol {
public:
    RFBProtocol(const std::string& host, const std::string& password, int port, double timeout);
    ~RFBProtocol();
    
    bool connect();
    void close();
    RFBResult getResult() const { return result; }
    
private:
    std::string host;
    std::string password;
    int port;
    double timeout;
    int sockfd;
    RFBResult result;
    bool is_ipv6;
    
    bool createSocket();
    bool connInit();
    bool clientAuth();
    bool vncAuth();
    bool clientInit();
    bool sendPassword(const uint8_t* challenge);
    bool receiveData(void* buffer, size_t len);
    bool sendData(const void* buffer, size_t len);
};

