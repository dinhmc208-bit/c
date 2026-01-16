#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

class ThreadPool {
public:
    ThreadPool(size_t num_threads);
    ~ThreadPool();
    
    template<typename F, typename... Args>
    void enqueue(F&& f, Args&&... args) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop) return;
            tasks.emplace([=]() { f(args...); });
        }
        condition.notify_one();
    }
    
    void waitAll();
    void stopAll();
    size_t getQueueSize() const;
    size_t getActiveThreads() const;
    
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    mutable std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
    std::atomic<size_t> active_threads;
    
    void worker();
};
