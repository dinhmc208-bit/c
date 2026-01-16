#pragma once

#include <string>
#include "config.h"
#include "scan_engine.h"
#include "brute_engine.h"

class CLI {
public:
    CLI(Config& config, ScanEngine& scan_engine, BruteEngine& brute_engine);
    void run();
    
private:
    Config& config;
    ScanEngine& scan_engine;
    BruteEngine& brute_engine;
    
    void printBanner();
    void printHelp();
    void processCommand(const std::string& line);
    void cmdScan(const std::string& args);
    void cmdBrute(const std::string& args);
    void cmdSet(const std::string& args);
    void cmdShow(const std::string& args);
    void cmdAdd(const std::string& args);
    void cmdFlush(const std::string& args);
    void cmdClear();
    void cmdExit();
    
    std::vector<std::string> split(const std::string& s, char delim);
    std::string trim(const std::string& s);
    void toLower(std::string& s);
};

