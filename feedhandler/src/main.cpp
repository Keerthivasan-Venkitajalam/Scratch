#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include "net/tcp_client.hpp"
#include "net/event_loop.hpp"
#include "net/receive_buffer.hpp"
#include "net/websocket_client.hpp"

int main(int argc, char* argv[]) {
    std::cout << "FeedHandler Boot OK" << std::endl;
    
    // Day 4: Receive Buffer with Fragmentation Handling
    std::cout << "\n=== Day 4: Receive Buffer Demo ===" << std::endl;
    {
        feedhandler::net::ReceiveBuffer buf;
        
        // Simulate fragmented TCP messages
        const char* msg1 = "Hello,";
        const char* msg2 = " World!";
        
        size_t w1 = buf.write(msg1, strlen(msg1));
        size_t w2 = buf.write(msg2, strlen(msg2));
        
        std::cout << "Written " << w1 << " bytes, then " << w2 << " bytes" << std::endl;
        std::cout << "Buffer has " << buf.readable_bytes() << " bytes readable" << std::endl;
        std::cout << "Data: " << std::string(buf.read_ptr(), buf.readable_bytes()) << std::endl;
        
        buf.consume(13);
        std::cout << "After consuming 13 bytes: " << buf.readable_bytes() << " bytes remain" << std::endl;
    }
    
    // Day 5: Connect to Real Feed
    std::cout << "\n=== Day 5: WebSocket Feed Connection Demo ===" << std::endl;
    
    if (argc > 1 && std::string(argv[1]) == "--feed") {
        feedhandler::net::WebSocketClient ws_client;
        
        // Try to connect to Binance (requires HTTPS/WSS, may fail without proper setup)
        std::cout << "Attempting to connect to Binance stream..." << std::endl;
        if (ws_client.connect_to_feed("/ws/btcusdt@trade", "stream.binance.com", 9443)) {
            std::cout << "Connected successfully" << std::endl;
            // ws_client.send_handshake();  // Would need SSL/TLS support
        } else {
            std::cout << "Connection failed (expected - requires SSL/TLS setup)" << std::endl;
            std::cout << "Binance WebSocket at: wss://stream.binance.com:9443/ws/btcusdt@trade" << std::endl;
        }
    } else {
        std::cout << "Run with --feed flag to attempt live feed connection" << std::endl;
        std::cout << "Example feed endpoints:" << std::endl;
        std::cout << "  Binance:  wss://stream.binance.com:9443/ws/btcusdt@trade" << std::endl;
        std::cout << "  Coinbase: wss://ws-feed.exchange.coinbase.com" << std::endl;
    }
    
    return 0;
}

