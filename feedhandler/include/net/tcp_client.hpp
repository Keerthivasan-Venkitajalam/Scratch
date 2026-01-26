#pragma once

#include <string>
#include <cstddef>

namespace feedhandler {
namespace net {

class TcpClient {
public:
    TcpClient();
    ~TcpClient();
    
    // Non-copyable
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;
    
    bool connect(const std::string& host, int port);
    bool send(const std::string& data);
    std::string recv(size_t max_bytes = 1024);
    void close();
    
    bool is_connected() const { return socket_fd_ != -1; }

private:
    int socket_fd_;
    bool connected_;
};

} // namespace net
} // namespace feedhandler