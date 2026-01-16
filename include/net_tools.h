#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>

struct IPRange {
    bool is_ipv6;
    union {
        struct {
            uint32_t start;
            uint32_t end;
        } v4;
        struct {
            uint8_t start[16];
            uint8_t end[16];
        } v6;
    };
    bool is_cidr;
    int cidr_prefix;
};

class NetTools {
public:
    static bool isIP(const std::string& address);
    static int getIPVersion(const std::string& address);
    static bool parseIP(const std::string& address, struct sockaddr_storage* addr);
    static bool parseRange(const std::string& range, IPRange* result);
    static bool parseCIDR(const std::string& cidr, IPRange* result);
    static std::string ipToString(const struct sockaddr_storage* addr);
    static uint32_t ipv4ToInt(const std::string& ip);
    static std::string intToIPv4(uint32_t ip);
    
    // Generate IP addresses from range
    static std::vector<std::string> generateIPs(const IPRange& range, size_t max_count = 1000000);
    
    // Check if string is a valid range
    static bool isRange(const std::string& range);
    
private:
    static bool parseIPv4(const std::string& ip, struct sockaddr_in* addr);
    static bool parseIPv6(const std::string& ip, struct sockaddr_in6* addr);
    static bool parseWildcard(const std::string& wildcard, IPRange* result);
};

