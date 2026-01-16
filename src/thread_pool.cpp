#include "thread_pool.h"
#include <iostream>
#include <thread>

ThreadPool::ThreadPool(size_t num_threads) : stop(false), active_threads(0) {
    // Don't artificially cap - let OS and config decide
    // Config already validates max is 65536
    for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back(&ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool() {
    stopAll();
}

void ThreadPool::stopAll() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::worker() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this] { return stop || !tasks.empty(); });
            
            if (stop && tasks.empty()) {
                return;
            }
            
            if (tasks.empty()) {
                continue;
            }
            
            task = std::move(tasks.front());
            tasks.pop();
            active_threads++;
        }
        
        if (task) {
            task();
        }
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            active_threads--;
        }
    }
}

void ThreadPool::waitAll() {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (tasks.empty() && active_threads == 0) {
                break;
            }
        }
        std::this_thread::yield();
    }
}

size_t ThreadPool::getQueueSize() const {
    std::unique_lock<std::mutex> lock(queue_mutex);
    return tasks.size();
}

size_t ThreadPool::getActiveThreads() const {
    return active_threads;
}
