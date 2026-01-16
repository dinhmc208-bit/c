#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <memory>
#include <fstream>
#include <mutex>
#include "rfb.h"
#include "thread_pool.h"

extern std::atomic<bool> g_running;

struct Server {
    std::string ip;
    int port;
};

class BruteEngine {
public:
    BruteEngine();
    ~BruteEngine();
    
    bool init(int max_threads, double timeout);
    void start();
    void stop();
    
    std::atomic<uint64_t> current{0};
    std::atomic<uint64_t> found{0};
    
private:
    std::vector<std::string> passwords;
    std::vector<Server> servers;
    double brute_timeout;
    std::unique_ptr<ThreadPool> thread_pool;
    std::ofstream results_file;
    std::mutex file_mutex;
    bool running;
    
    bool loadPasswords(const std::string& path);
    bool loadServers(const std::string& path);
    void bruteWorker(const Server& server, const std::string& password);
    void outputThread();
    void writeResult(const Server& server, const std::string& password, const RFBResult& result);
};

