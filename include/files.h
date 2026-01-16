#pragma once

#include <string>
#include <vector>
#include <fstream>

class FilesHandler {
public:
    FilesHandler();
    ~FilesHandler();
    
    bool fileExists(const std::string& path);
    bool dirExists(const std::string& path);
    bool mkdir(const std::string& path);
    std::string readFile(const std::string& path);
    bool writeFile(const std::string& path, const std::string& data, bool append = false);
    bool fileEmpty(const std::string& path);
    
    void deployFolders();
    void deployFiles();
    
    std::string getResultsPath() const { return results_path; }
    std::string getIPsPath() const { return ips_path; }
    std::string getPasswordsPath() const { return passwords_path; }
    std::string getConfigPath() const { return config_path; }
    
private:
    std::string root_path;
    std::string results_path;
    std::string ips_path;
    std::string passwords_path;
    std::string config_path;
    
    std::string joinPath(const std::string& a, const std::string& b);
};

