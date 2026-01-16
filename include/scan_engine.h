#pragma once

#include <string>
#include <atomic>
#include <memory>
#include <fstream>
#include "net_tools.h"
#include "thread_pool.h"
#include "io_multiplexer.h"

class ScanEngine {
public:
    ScanEngine();
    ~ScanEngine();
    
    bool init(const std::string& range, int port, double timeout, int max_threads);
    void start();
    void stop();
    
    std::atomic<uint64_t> current{0};
    std::atomic<uint64_t> found{0};
    std::atomic<uint64_t> total{0};
    
private:
    IPRange ip_range;
    int scan_port;
    double scan_timeout;
    std::unique_ptr<ThreadPool> thread_pool;
    std::unique_ptr<IOMultiplexer> io_multiplexer;
    std::ofstream ips_file;
    std::mutex file_mutex;
    bool running;
    
    void scanWorker(const std::string& ip);
    void outputThread();
    bool connectAndCheck(const std::string& ip, int port, double timeout);
    void writeResult(const std::string& ip, int port);
};

