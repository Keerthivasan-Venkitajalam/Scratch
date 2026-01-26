#include "net/tcp_client.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

namespace feedhandler {
namespace net {

TcpClient::TcpClient() : socket_fd_(-1), connected_(false) {
}

TcpClient::~TcpClient() {
    close();
}

bool TcpClient::connect(const std::string& host, int port) {
    // Create socket
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Resolve hostname
    struct hostent* server = gethostbyname(host.c_str());
    if (server == nullptr) {
        std::cerr << "Failed to resolve host: " << host << std::endl;
        ::close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    // Setup server address
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    std::memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    // Connect
    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to " << host << ":" << port 
                  << " - " << strerror(errno) << std::endl;
        ::close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    connected_ = true;
    std::cout << "Connected to " << host << ":" << port << std::endl;
    return true;
}

bool TcpClient::send(const std::string& data) {
    if (!connected_ || socket_fd_ < 0) {
        std::cerr << "Not connected" << std::endl;
        return false;
    }
    
    ssize_t bytes_sent = ::send(socket_fd_, data.c_str(), data.length(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Send failed: " << strerror(errno) << std::endl;
        return false;
    }
    
    if (static_cast<size_t>(bytes_sent) != data.length()) {
        std::cerr << "Partial send: " << bytes_sent << "/" << data.length() << " bytes" << std::endl;
        return false;
    }
    
    return true;
}

std::string TcpClient::recv(size_t max_bytes) {
    if (!connected_ || socket_fd_ < 0) {
        std::cerr << "Not connected" << std::endl;
        return "";
    }
    
    std::string buffer(max_bytes, '\0');
    ssize_t bytes_received = ::recv(socket_fd_, buffer.data(), max_bytes - 1, 0);
    
    if (bytes_received < 0) {
        std::cerr << "Receive failed: " << strerror(errno) << std::endl;
        return "";
    }
    
    if (bytes_received == 0) {
        std::cout << "Connection closed by peer" << std::endl;
        connected_ = false;
        return "";
    }
    
    buffer.resize(bytes_received);
    return buffer;
}

void TcpClient::close() {
    if (socket_fd_ >= 0) {
        ::close(socket_fd_);
        socket_fd_ = -1;
        connected_ = false;
    }
}

} // namespace net
} // namespace feedhandler