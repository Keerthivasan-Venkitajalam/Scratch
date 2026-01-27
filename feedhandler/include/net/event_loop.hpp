#pragma once

#include <sys/select.h>
#include <vector>

namespace feedhandler {
namespace net {

class EventLoop {
public:
    EventLoop();
    ~EventLoop();
    
    void add_socket(int sock);
    void remove_socket(int sock);
    
    // Run one select cycle with timeout (milliseconds)
    // Returns true if socket became readable, false on timeout
    bool run_once(int timeout_ms = 1000);
    
    bool is_readable(int sock) const;
    
private:
    fd_set readfds_;
    int max_fd_;
    std::vector<int> sockets_;
};

} // namespace net
} // namespace feedhandler
