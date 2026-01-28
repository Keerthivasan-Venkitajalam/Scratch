#include "net/websocket_client.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

namespace feedhandler {
namespace net {

WebSocketClient::WebSocketClient() : socket_fd_(-1) {}

WebSocketClient::~WebSocketClient() {
    close();
}

bool WebSocketClient::connect_to_feed(const std::string& /* url */, const std::string& host, int port) {
    host_ = host;
    
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0) {
        std::cerr << "Failed to resolve host: " << host << std::endl;
        return false;
    }
    
    socket_fd_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socket_fd_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        freeaddrinfo(res);
        return false;
    }
    
    if (connect(socket_fd_, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "Failed to connect to " << host << ":" << port << std::endl;
        close();
        freeaddrinfo(res);
        return false;
    }
    
    freeaddrinfo(res);
    std::cout << "Connected to " << host << ":" << port << std::endl;
    return true;
}

bool WebSocketClient::send_handshake() {
    if (socket_fd_ < 0) return false;
    
    // Simple WebSocket handshake
    std::string handshake = 
        "GET /ws/btcusdt@trade HTTP/1.1\r\n"
        "Host: stream.binance.com:9443\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: SGVsbG8sIHdvcmxkIQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n";
    
    if (send(socket_fd_, handshake.c_str(), handshake.length(), 0) < 0) {
        std::cerr << "Failed to send handshake" << std::endl;
        return false;
    }
    
    std::cout << "Handshake sent" << std::endl;
    return true;
}

std::string WebSocketClient::recv_data() {
    if (socket_fd_ < 0) return "";
    
    char buffer[4096];
    ssize_t n = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0);
    
    if (n > 0) {
        buffer[n] = '\0';
        return std::string(buffer);
    }
    
    return "";
}

void WebSocketClient::close() {
    if (socket_fd_ >= 0) {
        ::close(socket_fd_);
        socket_fd_ = -1;
    }
}

} // namespace net
} // namespace feedhandler
