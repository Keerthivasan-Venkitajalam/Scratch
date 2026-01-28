#pragma once

#include <string>
#include <memory>

namespace feedhandler {
namespace net {

// Simple WebSocket client for connecting to Binance/Coinbase feeds
class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();
    
    // Connect to WebSocket endpoint
    bool connect_to_feed(const std::string& url, const std::string& host, int port);
    
    // Send WebSocket upgrade handshake
    bool send_handshake();
    
    // Receive raw bytes (WebSocket frames or plain JSON if plain TCP)
    std::string recv_data();
    
    bool is_connected() const { return socket_fd_ >= 0; }
    
    void close();
    
private:
    int socket_fd_;
    std::string host_;
};

} // namespace net
} // namespace feedhandler
