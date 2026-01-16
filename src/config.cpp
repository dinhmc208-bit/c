#include "config.h"
#include <iostream>
#include <algorithm>
#include <cctype>

Config::Config() {
    setDefault();
}

void Config::setDefault() {
    scan_range = "192.168.*.*";
    scan_port = 5900;
    scan_timeout = 15.0;
    scan_threads = 1000;
    brute_threads = 1000;
    brute_timeout = 15.0;
    auto_save = true;
    auto_brute = true;
}

int Config::clampThreads(int value) {
    const int MAX_THREADS = 65536;  // Allow up to 65k worker threads (modern OS can handle)
    const int MIN_THREADS = 1;
    if (value < MIN_THREADS) return MIN_THREADS;
    if (value > MAX_THREADS) return MAX_THREADS;
    return value;
}

bool Config::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        parseLine(line);
    }
    
    file.close();
    return true;
}

void Config::parseLine(const std::string& line) {
    size_t pos = line.find('=');
    if (pos == std::string::npos) return;
    
    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);
    
    // Trim whitespace
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);
    
    config_map[key] = value;
    
    // Parse values
    if (key == "scan_range") {
        scan_range = value;
    } else if (key == "scan_port") {
        scan_port = std::stoi(value);
    } else if (key == "scan_timeout") {
        scan_timeout = std::stod(value);
    } else if (key == "scan_threads") {
        scan_threads = clampThreads(std::stoi(value));
    } else if (key == "brute_threads") {
        brute_threads = clampThreads(std::stoi(value));
    } else if (key == "brute_timeout") {
        brute_timeout = std::stod(value);
    } else if (key == "auto_save") {
        auto_save = (value == "true" || value == "1");
    } else if (key == "auto_brute") {
        auto_brute = (value == "true" || value == "1");
    }
}

bool Config::save(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    file << "scan_range=" << scan_range << "\n";
    file << "scan_port=" << scan_port << "\n";
    file << "scan_timeout=" << scan_timeout << "\n";
    file << "scan_threads=" << scan_threads << "\n";
    file << "brute_threads=" << brute_threads << "\n";
    file << "brute_timeout=" << brute_timeout << "\n";
    file << "auto_save=" << (auto_save ? "true" : "false") << "\n";
    file << "auto_brute=" << (auto_brute ? "true" : "false") << "\n";
    
    file.close();
    return true;
}

