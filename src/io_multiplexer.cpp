#include "io_multiplexer.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <algorithm>
#include <ctime>

#ifdef USE_EPOLL
#include <sys/epoll.h>
#endif

#ifdef USE_KQUEUE
#include <sys/event.h>
#endif

IOMultiplexer::IOMultiplexer(size_t max_connections) 
    : epoll_fd(-1), kqueue_fd(-1), use_epoll(false), use_kqueue(false), max_connections(max_connections) {
    
#ifdef USE_EPOLL
    setupEpoll();
#elif defined(USE_KQUEUE)
    setupKqueue();
#endif
}

IOMultiplexer::~IOMultiplexer() {
    if (epoll_fd >= 0) {
        close(epoll_fd);
    }
    if (kqueue_fd >= 0) {
        close(kqueue_fd);
    }
}

void IOMultiplexer::setupEpoll() {
#ifdef USE_EPOLL
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd >= 0) {
        use_epoll = true;
    }
#endif
}

void IOMultiplexer::setupKqueue() {
#ifdef USE_KQUEUE
    kqueue_fd = kqueue();
    if (kqueue_fd >= 0) {
        use_kqueue = true;
    }
#endif
}

bool IOMultiplexer::addSocket(int fd, const struct sockaddr_storage* addr, socklen_t addrlen,
                              std::function<void(int, bool)> callback, double timeout) {
    if (tasks.size() >= max_connections) {
        return false;
    }
    
    // Set non-blocking
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    
    SocketTask task;
    task.fd = fd;
    memcpy(&task.addr, addr, addrlen);
    task.addrlen = addrlen;
    task.callback = callback;
    task.timeout = timeout;
    task.start_time = time(nullptr);
    
    tasks.push_back(task);
    fds.push_back(fd);
    
    if (use_epoll) {
#ifdef USE_EPOLL
        struct epoll_event ev;
        ev.events = EPOLLOUT | EPOLLET;
        ev.data.fd = fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            return false;
        }
#endif
    } else if (use_kqueue) {
#ifdef USE_KQUEUE
        struct kevent kev;
        EV_SET(&kev, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, nullptr);
        if (kevent(kqueue_fd, &kev, 1, nullptr, 0, nullptr) < 0) {
            return false;
        }
#endif
    }
    
    return true;
}

void IOMultiplexer::removeSocket(int fd) {
    if (use_epoll) {
#ifdef USE_EPOLL
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
#endif
    } else if (use_kqueue) {
#ifdef USE_KQUEUE
        struct kevent kev;
        EV_SET(&kev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
        kevent(kqueue_fd, &kev, 1, nullptr, 0, nullptr);
#endif
    }
    
    auto it = std::find(fds.begin(), fds.end(), fd);
    if (it != fds.end()) {
        fds.erase(it);
    }
    
    tasks.erase(
        std::remove_if(tasks.begin(), tasks.end(),
            [fd](const SocketTask& t) { return t.fd == fd; }),
        tasks.end()
    );
    
    close(fd);
}

void IOMultiplexer::processEvents(int timeout_ms) {
    checkTimeouts();
    
    if (use_epoll) {
        processEpoll(timeout_ms);
    } else if (use_kqueue) {
        processKqueue(timeout_ms);
    }
}

void IOMultiplexer::processEpoll(int timeout_ms) {
#ifdef USE_EPOLL
    const int MAX_EVENTS = 1000;
    struct epoll_event events[MAX_EVENTS];
    
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, timeout_ms);
    
    for (int i = 0; i < nfds; i++) {
        int fd = events[i].data.fd;
        
        auto it = std::find_if(tasks.begin(), tasks.end(),
            [fd](const SocketTask& t) { return t.fd == fd; });
        
        if (it != tasks.end()) {
            bool success = (events[i].events & EPOLLOUT) != 0;
            it->callback(fd, success);
            removeSocket(fd);
        }
    }
#endif
}

void IOMultiplexer::processKqueue(int timeout_ms) {
    (void)timeout_ms;  // Used in kqueue branch
#ifdef USE_KQUEUE
    const int MAX_EVENTS = 1000;
    struct kevent events[MAX_EVENTS];
    
    struct timespec timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_nsec = (timeout_ms % 1000) * 1000000;
    
    int nfds = kevent(kqueue_fd, nullptr, 0, events, MAX_EVENTS, &timeout);
    
    for (int i = 0; i < nfds; i++) {
        int fd = events[i].ident;
        
        auto it = std::find_if(tasks.begin(), tasks.end(),
            [fd](const SocketTask& t) { return t.fd == fd; });
        
        if (it != tasks.end()) {
            bool success = (events[i].filter == EVFILT_WRITE);
            it->callback(fd, success);
            removeSocket(fd);
        }
    }
#endif
}

void IOMultiplexer::checkTimeouts() {
    time_t now = time(nullptr);
    
    auto it = tasks.begin();
    while (it != tasks.end()) {
        if (now - it->start_time > static_cast<time_t>(it->timeout)) {
            it->callback(it->fd, false);
            int fd = it->fd;
            ++it;
            removeSocket(fd);
        } else {
            ++it;
        }
    }
}

size_t IOMultiplexer::getActiveConnections() const {
    return tasks.size();
}

