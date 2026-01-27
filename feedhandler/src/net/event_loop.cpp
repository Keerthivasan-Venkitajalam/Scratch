#include "net/event_loop.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <algorithm>

namespace feedhandler {
namespace net {

EventLoop::EventLoop() : max_fd_(0) {
    FD_ZERO(&readfds_);
}

EventLoop::~EventLoop() {}

void EventLoop::add_socket(int sock) {
    if (sock < 0) return;
    
    sockets_.push_back(sock);
    if (sock > max_fd_) {
        max_fd_ = sock;
    }
    FD_SET(sock, &readfds_);
}

void EventLoop::remove_socket(int sock) {
    auto it = std::find(sockets_.begin(), sockets_.end(), sock);
    if (it != sockets_.end()) {
        sockets_.erase(it);
    }
    FD_CLR(sock, &readfds_);
}

bool EventLoop::run_once(int timeout_ms) {
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    fd_set test_set = readfds_;  // select() modifies fd_set, so copy
    
    int activity = select(max_fd_ + 1, &test_set, nullptr, nullptr, &tv);
    
    if (activity > 0) {
        // Copy back the modified set
        readfds_ = test_set;
        std::cout << "[EventLoop] Socket readable" << std::endl;
        return true;
    } else if (activity == 0) {
        std::cout << "[EventLoop] Timeout — no data" << std::endl;
        return false;
    } else {
        std::cout << "[EventLoop] Select error" << std::endl;
        return false;
    }
}

bool EventLoop::is_readable(int sock) const {
    return FD_ISSET(sock, &readfds_);
}

} // namespace net
} // namespace feedhandler
