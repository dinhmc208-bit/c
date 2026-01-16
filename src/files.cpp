#include "files.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <iostream>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path.c_str())
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

FilesHandler::FilesHandler() {
    root_path = "./";
    
    results_path = joinPath(joinPath(root_path, "output"), "results.txt");
    ips_path = joinPath(joinPath(root_path, "output"), "ips.txt");
    passwords_path = joinPath(joinPath(root_path, "input"), "passwords.txt");
    config_path = joinPath(joinPath(root_path, "bin"), "config.conf");
}

FilesHandler::~FilesHandler() {
}

std::string FilesHandler::joinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    
    char sep = '/';
#ifdef _WIN32
    sep = '\\';
#endif
    
    if (a.back() == sep) {
        return a + b;
    }
    return a + sep + b;
}

bool FilesHandler::fileExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

bool FilesHandler::dirExists(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

bool FilesHandler::mkdir(const std::string& path) {
    if (dirExists(path)) {
        return true;
    }
    
#ifdef _WIN32
    return _mkdir(path.c_str()) == 0;
#else
    return ::mkdir(path.c_str(), 0755) == 0;
#endif
}

std::string FilesHandler::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool FilesHandler::writeFile(const std::string& path, const std::string& data, bool append) {
    std::ofstream file(path, append ? std::ios::app : std::ios::out);
    if (!file.is_open()) {
        return false;
    }
    
    file << data;
    file.close();
    return true;
}

bool FilesHandler::fileEmpty(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return true;
    }
    
    file.seekg(0, std::ios::end);
    return file.tellg() == 0;
}

void FilesHandler::deployFolders() {
    mkdir(joinPath(root_path, "output"));
    mkdir(joinPath(root_path, "input"));
    mkdir(joinPath(root_path, "bin"));
}

void FilesHandler::deployFiles() {
    // Create files if they don't exist
    if (!fileExists(results_path)) {
        writeFile(results_path, "");
    }
    if (!fileExists(ips_path)) {
        writeFile(ips_path, "");
    }
    if (!fileExists(passwords_path)) {
        writeFile(passwords_path, "1\n12\n123\n1234\n12345\n123456\n1234567\n12345678\nletmein\nadmin\nadminist\npassword\n1212\n");
    }
    if (!fileExists(config_path)) {
        writeFile(config_path, "");
    }
}

