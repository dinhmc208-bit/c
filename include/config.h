#pragma once

#include <string>
#include <map>
#include <fstream>
#include <sstream>

class Config {
public:
    std::string scan_range;
    int scan_port;
    double scan_timeout;
    int scan_threads;
    int brute_threads;
    double brute_timeout;
    bool auto_save;
    bool auto_brute;

    Config();
    bool load(const std::string& path);
    bool save(const std::string& path);
    void setDefault();
    static int clampThreads(int value);
    
private:
    std::map<std::string, std::string> config_map;
    void parseLine(const std::string& line);
};

