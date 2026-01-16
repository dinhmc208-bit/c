#include "net_tools.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <sstream>
#include <algorithm>

bool NetTools::isIP(const std::string& address) {
    struct sockaddr_storage addr;
    return parseIP(address, &addr);
}

int NetTools::getIPVersion(const std::string& address) {
    struct sockaddr_storage addr;
    if (!parseIP(address, &addr)) {
        return 0;
    }
    return addr.ss_family == AF_INET6 ? 6 : 4;
}

bool NetTools::parseIP(const std::string& address, struct sockaddr_storage* addr) {
    if (parseIPv4(address, reinterpret_cast<struct sockaddr_in*>(addr))) {
        return true;
    }
    if (parseIPv6(address, reinterpret_cast<struct sockaddr_in6*>(addr))) {
        return true;
    }
    return false;
}

bool NetTools::parseIPv4(const std::string& ip, struct sockaddr_in* addr) {
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    return inet_pton(AF_INET, ip.c_str(), &addr->sin_addr) == 1;
}

bool NetTools::parseIPv6(const std::string& ip, struct sockaddr_in6* addr) {
    memset(addr, 0, sizeof(*addr));
    addr->sin6_family = AF_INET6;
    return inet_pton(AF_INET6, ip.c_str(), &addr->sin6_addr) == 1;
}

bool NetTools::parseRange(const std::string& range, IPRange* result) {
    // Try CIDR first
    if (parseCIDR(range, result)) {
        return true;
    }
    
    // Try range with dash
    size_t dash_pos = range.find('-');
    if (dash_pos != std::string::npos) {
        std::string start_str = range.substr(0, dash_pos);
        std::string end_str = range.substr(dash_pos + 1);
        
        struct sockaddr_storage start_addr, end_addr;
        if (parseIP(start_str, &start_addr) && parseIP(end_str, &end_addr)) {
            if (start_addr.ss_family == AF_INET && end_addr.ss_family == AF_INET) {
                result->is_ipv6 = false;
                result->v4.start = ntohl(reinterpret_cast<struct sockaddr_in*>(&start_addr)->sin_addr.s_addr);
                result->v4.end = ntohl(reinterpret_cast<struct sockaddr_in*>(&end_addr)->sin_addr.s_addr);
                result->is_cidr = false;
                return true;
            }
        }
    }
    
    // Try wildcard (IPv4 only)
    return parseWildcard(range, result);
}

bool NetTools::parseCIDR(const std::string& cidr, IPRange* result) {
    size_t slash_pos = cidr.find('/');
    if (slash_pos == std::string::npos) {
        return false;
    }
    
    std::string ip_str = cidr.substr(0, slash_pos);
    std::string prefix_str = cidr.substr(slash_pos + 1);
    
    struct sockaddr_storage addr;
    if (!parseIP(ip_str, &addr)) {
        return false;
    }
    
    int prefix = std::stoi(prefix_str);
    
    if (addr.ss_family == AF_INET) {
        result->is_ipv6 = false;
        uint32_t ip = ntohl(reinterpret_cast<struct sockaddr_in*>(&addr)->sin_addr.s_addr);
        uint32_t mask = (0xFFFFFFFF << (32 - prefix)) & 0xFFFFFFFF;
        result->v4.start = ip & mask;
        result->v4.end = ip | (~mask);
        result->is_cidr = true;
        result->cidr_prefix = prefix;
        return true;
    } else if (addr.ss_family == AF_INET6) {
        result->is_ipv6 = true;
        memcpy(result->v6.start, &reinterpret_cast<struct sockaddr_in6*>(&addr)->sin6_addr, 16);
        // Calculate end address (simplified)
        memcpy(result->v6.end, result->v6.start, 16);
        result->is_cidr = true;
        result->cidr_prefix = prefix;
        return true;
    }
    
    return false;
}

bool NetTools::parseWildcard(const std::string& wildcard, IPRange* result) {
    size_t star_count = std::count(wildcard.begin(), wildcard.end(), '*');
    if (star_count == 0 || star_count > 3) {
        return false;
    }
    
    std::string test_ip = wildcard;
    std::replace(test_ip.begin(), test_ip.end(), '*', '0');
    
    struct sockaddr_in addr;
    if (parseIPv4(test_ip, &addr)) {
        std::string start_str = wildcard;
        std::string end_str = wildcard;
        std::replace(start_str.begin(), start_str.end(), '*', '0');
        
        // Replace wildcards with 255 in end_str
        size_t pos = 0;
        while ((pos = end_str.find('*', pos)) != std::string::npos) {
            end_str.replace(pos, 1, "255");
            pos += 3;  // Account for the 3-char replacement
        }
        
        result->is_ipv6 = false;
        result->v4.start = ipv4ToInt(start_str);
        result->v4.end = ipv4ToInt(end_str);
        result->is_cidr = false;
        return true;
    }
    
    return false;
}

std::string NetTools::ipToString(const struct sockaddr_storage* addr) {
    char buffer[INET6_ADDRSTRLEN];
    
    if (addr->ss_family == AF_INET) {
        inet_ntop(AF_INET, &reinterpret_cast<const struct sockaddr_in*>(addr)->sin_addr,
                  buffer, INET_ADDRSTRLEN);
    } else if (addr->ss_family == AF_INET6) {
        inet_ntop(AF_INET6, &reinterpret_cast<const struct sockaddr_in6*>(addr)->sin6_addr,
                  buffer, INET6_ADDRSTRLEN);
    } else {
        return "";
    }
    
    return std::string(buffer);
}

uint32_t NetTools::ipv4ToInt(const std::string& ip) {
    struct sockaddr_in addr;
    if (parseIPv4(ip, &addr)) {
        return ntohl(addr.sin_addr.s_addr);
    }
    return 0;
}

std::string NetTools::intToIPv4(uint32_t ip) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(ip);
    
    char buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, buffer, INET_ADDRSTRLEN);
    return std::string(buffer);
}

std::vector<std::string> NetTools::generateIPs(const IPRange& range, size_t max_count) {
    std::vector<std::string> ips;
    
    if (!range.is_ipv6) {
        uint32_t start = range.v4.start;
        uint32_t end = range.v4.end;
        
        size_t count = 0;
        for (uint32_t ip = start; ip <= end && count < max_count; ip++, count++) {
            ips.push_back(intToIPv4(ip));
        }
    } else {
        // IPv6: simplified - just return start and end
        struct sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        memcpy(&addr.sin6_addr, range.v6.start, 16);
        ips.push_back(ipToString(reinterpret_cast<struct sockaddr_storage*>(&addr)));
    }
    
    return ips;
}

bool NetTools::isRange(const std::string& range) {
    IPRange result;
    return parseRange(range, &result);
}

