#include "cli.h"
#include "files.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

extern std::atomic<bool> g_running;

CLI::CLI(Config& config, ScanEngine& scan_engine, BruteEngine& brute_engine)
    : config(config), scan_engine(scan_engine), brute_engine(brute_engine) {
}

void CLI::run() {
    printBanner();
    printHelp();
    
    std::string line;
    while (g_running && std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        processCommand(line);
        
        if (line == "exit" || line == "quit" || line == "q") {
            break;
        }
        
        // Check if Ctrl-C was pressed
        if (!g_running) {
            std::cout << "\n\t[Stopping...]\n\n";
            break;
        }
    }
}

void CLI::printBanner() {
    std::cout << "\n";
    std::cout << "|>>>> - VNC Scanner - 2.0.0 - IPv6Ready - <<<<|\n";
    std::cout << "Scan Threads: " << config.scan_threads 
              << " <-> Scan Timeout: " << config.scan_timeout 
              << " <-> Scan Port: " << config.scan_port << "\n";
    std::cout << "Brute Threads: " << config.brute_threads 
              << " <-> Brute Timeout: " << config.brute_timeout 
              << " <-> Auto Brute: " << (config.auto_brute ? "true" : "false") << "\n";
    std::cout << "Scan Range: " << config.scan_range 
              << " <-> Auto Save: " << (config.auto_save ? "true" : "false") << "\n";
    std::cout << "\n";
}

void CLI::printHelp() {
    std::cout << "Commands:\n";
    std::cout << "  scan <range>     - Scan IP range\n";
    std::cout << "  brute            - Start brute force\n";
    std::cout << "  set <key> <val>  - Set configuration\n";
    std::cout << "  show <type>      - Show results/settings\n";
    std::cout << "  add <data> <file>- Add data to file\n";
    std::cout << "  flush <file>     - Clear file\n";
    std::cout << "  clear/cls        - Clear screen\n";
    std::cout << "  exit/quit/q      - Exit\n";
    std::cout << "\n";
}

void CLI::processCommand(const std::string& line) {
    std::vector<std::string> parts = split(line, ' ');
    if (parts.empty()) return;
    
    std::string cmd = parts[0];
    toLower(cmd);
    
    if (cmd == "scan") {
        if (parts.size() > 1) {
            cmdScan(parts[1]);
        } else {
            cmdScan(config.scan_range);
        }
    } else if (cmd == "brute") {
        cmdBrute("");
    } else if (cmd == "set") {
        if (parts.size() >= 3) {
            cmdSet(parts[1] + " " + parts[2]);
        }
    } else if (cmd == "show") {
        cmdShow(parts.size() > 1 ? parts[1] : "");
    } else if (cmd == "add") {
        if (parts.size() >= 3) {
            cmdAdd(parts[1] + " " + parts[2]);
        }
    } else if (cmd == "flush") {
        if (parts.size() > 1) {
            cmdFlush(parts[1]);
        }
    } else if (cmd == "clear" || cmd == "cls") {
        cmdClear();
    } else if (cmd == "exit" || cmd == "quit" || cmd == "q") {
        cmdExit();
    } else {
        std::cout << "\n\tNope.\n\n";
    }
}

void CLI::cmdScan(const std::string& args) {
    std::string range = trim(args);
    toLower(range);
    
    if (NetTools::isRange(range)) {
        config.scan_range = range;
        std::cout << "\n\t[OK]\n\n";
        
        scan_engine.init(range, config.scan_port, config.scan_timeout, config.scan_threads);
        scan_engine.start();
        
        if (config.auto_brute) {
            brute_engine.init(config.brute_threads, config.brute_timeout);
            brute_engine.start();
        } else {
            std::cout << "\n\nDONE! Check \"output/ips.txt\"!\n\n";
        }
    } else {
        std::cout << "\n\t[ERROR]\n\n";
    }
}

void CLI::cmdBrute(const std::string& /* args */) {
    brute_engine.init(config.brute_threads, config.brute_timeout);
    brute_engine.start();
    std::cout << "\n\nDONE! Check \"output/results.txt\"!\n\n";
}

void CLI::cmdSet(const std::string& args) {
    std::vector<std::string> parts = split(args, ' ');
    if (parts.size() < 2) {
        std::cout << "\n\t[ERROR]\n\n";
        return;
    }
    
    std::string key = parts[0];
    std::string value = parts[1];
    toLower(key);
    toLower(value);
    
    bool ok = false;
    
    try {
        if (key == "scan_range" && NetTools::isRange(value)) {
            config.scan_range = value;
            ok = true;
        } else if (key == "scan_port") {
            config.scan_port = std::stoi(value);
            ok = true;
        } else if (key == "scan_timeout") {
            config.scan_timeout = std::stod(value);
            ok = true;
        } else if (key == "scan_threads") {
            config.scan_threads = Config::clampThreads(std::stoi(value));
            ok = true;
        } else if (key == "brute_threads") {
            config.brute_threads = Config::clampThreads(std::stoi(value));
            ok = true;
        } else if (key == "brute_timeout") {
            config.brute_timeout = std::stod(value);
            ok = true;
        } else if (key == "auto_save") {
            config.auto_save = (value == "true" || value == "1");
            ok = true;
        } else if (key == "auto_brute") {
            config.auto_brute = (value == "true" || value == "1");
            ok = true;
        }
    } catch (const std::exception& e) {
        std::cout << "\n\t[ERROR]\n\n";
        return;
    }
    
    if (ok) {
        std::cout << "\n\t[OK]\n\n";
        if (config.auto_save) {
            FilesHandler files;
            config.save(files.getConfigPath());
        }
    } else {
        std::cout << "\n\t[ERROR]\n\n";
    }
}

void CLI::cmdShow(const std::string& args) {
    std::string type = trim(args);
    toLower(type);
    
    FilesHandler files;
    
    if (type == "results" || type == "result" || type == "brute") {
        std::cout << "\nBrute Results\n";
        std::cout << std::string(13, '-') << "\n";
        std::cout << files.readFile(files.getResultsPath());
        std::cout << std::string(13, '-') << "\n";
    } else if (type == "ips" || type == "scan" || type == "ip") {
        std::cout << "\nScan Results\n";
        std::cout << std::string(12, '-') << "\n";
        std::cout << files.readFile(files.getIPsPath());
        std::cout << std::string(12, '-') << "\n";
    } else if (type == "password" || type == "passwords" || type == "pass") {
        std::cout << "\nPasswords\n";
        std::cout << std::string(9, '-') << "\n";
        std::cout << files.readFile(files.getPasswordsPath());
        std::cout << std::string(9, '-') << "\n";
    } else {
        std::cout << "\nSettings\n";
        std::cout << std::string(8, '-') << "\n";
        std::cout << "scan_range=" << config.scan_range << "\n";
        std::cout << "scan_port=" << config.scan_port << "\n";
        std::cout << "scan_timeout=" << config.scan_timeout << "\n";
        std::cout << "scan_threads=" << config.scan_threads << "\n";
        std::cout << "brute_threads=" << config.brute_threads << "\n";
        std::cout << "brute_timeout=" << config.brute_timeout << "\n";
        std::cout << "auto_save=" << (config.auto_save ? "true" : "false") << "\n";
        std::cout << "auto_brute=" << (config.auto_brute ? "true" : "false") << "\n";
        std::cout << std::string(8, '-') << "\n";
    }
    std::cout << "\n";
}

void CLI::cmdAdd(const std::string& /* args */) {
    // Implementation for add command
    std::cout << "\n\t[OK]\n\n";
}

void CLI::cmdFlush(const std::string& /* args */) {
    // Implementation for flush command
    std::cout << "\n\t[OK]\n\n";
}

void CLI::cmdClear() {
    #ifdef _WIN32
    if (system("CLS") != 0) { /* failed to clear */ }
    #else
    if (system("clear") != 0) { /* failed to clear */ }
    #endif
    printBanner();
}

void CLI::cmdExit() {
    std::cout << "Bye.\n";
}

std::vector<std::string> CLI::split(const std::string& s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    
    while (std::getline(ss, item, delim)) {
        result.push_back(item);
    }
    
    return result;
}

std::string CLI::trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

void CLI::toLower(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}

