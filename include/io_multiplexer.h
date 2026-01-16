#pragma once

#include <vector>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory>

struct SocketTask {
    int fd;
    struct sockaddr_storage addr;
    socklen_t addrlen;
    std::function<void(int, bool)> callback;
    double timeout;
    time_t start_time;
};

class IOMultiplexer {
public:
    IOMultiplexer(size_t max_connections = 100000);
    ~IOMultiplexer();
    
    bool addSocket(int fd, const struct sockaddr_storage* addr, socklen_t addrlen,
                   std::function<void(int, bool)> callback, double timeout);
    void removeSocket(int fd);
    void processEvents(int timeout_ms = 100);
    size_t getActiveConnections() const;
    
private:
    int epoll_fd;
    int kqueue_fd;
    bool use_epoll;
    bool use_kqueue;
    std::vector<SocketTask> tasks;
    std::vector<int> fds;
    size_t max_connections;
    
    void setupEpoll();
    void setupKqueue();
    void processEpoll(int timeout_ms);
    void processKqueue(int timeout_ms);
    void checkTimeouts();
};

