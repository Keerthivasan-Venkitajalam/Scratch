// Final FeedHandler Demo - Complete Pipeline
// Connects to mock FIX server, parses messages, displays bid/ask spread

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include "parser/fsm_fix_parser.hpp"
#include "net/receive_buffer.hpp"
#include "common/tick.hpp"

using namespace feedhandler;

class OrderBook {
public:
    void update(const common::Tick& tick) {
        std::string symbol(tick.symbol.data(), tick.symbol.size());
        
        if (tick.side == 'B' || tick.side == '1') {
            // Bid (buy order)
            bids_[symbol] = tick;
        } else if (tick.side == 'S' || tick.side == '2') {
            // Ask (sell order)
            asks_[symbol] = tick;
        }
    }
    
    void print_spread(const std::string& symbol) const {
        auto bid_it = bids_.find(symbol);
        auto ask_it = asks_.find(symbol);
        
        if (bid_it != bids_.end() && ask_it != asks_.end()) {
            double bid_price = common::price_to_double(bid_it->second.price);
            double ask_price = common::price_to_double(ask_it->second.price);
            double spread = ask_price - bid_price;
            double spread_bps = (spread / bid_price) * 10000.0;
            
            std::cout << "\n┌─────────────────────────────────────────┐" << std::endl;
            std::cout << "│  " << symbol << " Order Book                        │" << std::endl;
            std::cout << "├─────────────────────────────────────────┤" << std::endl;
            std::cout << "│  Bid: $" << bid_price << " x " << bid_it->second.qty << "                │" << std::endl;
            std::cout << "│  Ask: $" << ask_price << " x " << ask_it->second.qty << "                │" << std::endl;
            std::cout << "├─────────────────────────────────────────┤" << std::endl;
            std::cout << "│  Spread: $" << spread << " (" << spread_bps << " bps)      │" << std::endl;
            std::cout << "└─────────────────────────────────────────┘" << std::endl;
        }
    }
    
    void print_all_spreads() const {
        for (const auto& [symbol, _] : bids_) {
            print_spread(symbol);
        }
    }
    
private:
    std::map<std::string, common::Tick> bids_;
    std::map<std::string, common::Tick> asks_;
};

class FeedHandler {
public:
    FeedHandler() : socket_fd_(-1) {
        parser_.set_garbage_recovery(true);
    }
    
    ~FeedHandler() {
        disconnect();
    }
    
    bool connect(const std::string& host, int port) {
        // Create socket
        socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd_ < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }
        
        // Set non-blocking
        int flags = fcntl(socket_fd_, F_GETFL, 0);
        fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK);
        
        // Connect
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address: " << host << std::endl;
            close(socket_fd_);
            return false;
        }
        
        // Non-blocking connect
        int result = ::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (result < 0 && errno != EINPROGRESS) {
            std::cerr << "Failed to connect to " << host << ":" << port << std::endl;
            close(socket_fd_);
            return false;
        }
        
        // Wait for connection (simple blocking wait for demo)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "Connected to " << host << ":" << port << std::endl;
        return true;
    }
    
    void disconnect() {
        if (socket_fd_ >= 0) {
            close(socket_fd_);
            socket_fd_ = -1;
        }
    }
    
    void run() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "FeedHandler Running" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "\nReceiving market data...\n" << std::endl;
        
        char temp_buffer[4096];
        std::vector<common::Tick> ticks;
        ticks.reserve(100);
        
        int message_count = 0;
        
        while (true) {
            // Read from socket
            ssize_t bytes_read = recv(socket_fd_, temp_buffer, sizeof(temp_buffer), 0);
            
            if (bytes_read > 0) {
                // Write to receive buffer
                buffer_.write(temp_buffer, bytes_read);
                
                // Parse available data
                size_t consumed = parser_.parse(
                    buffer_.read_ptr(), 
                    buffer_.readable_bytes(), 
                    ticks
                );
                
                buffer_.consume(consumed);
                
                // Process ticks
                for (const auto& tick : ticks) {
                    message_count++;
                    
                    std::string symbol(tick.symbol.data(), tick.symbol.size());
                    double price = common::price_to_double(tick.price);
                    char side_char = (tick.side == '1' || tick.side == 'B') ? 'B' : 'A';
                    
                    std::cout << "[" << message_count << "] " 
                              << symbol << " " 
                              << side_char << " "
                              << "$" << price << " x " << tick.qty 
                              << std::endl;
                    
                    // Update order book
                    order_book_.update(tick);
                }
                
                ticks.clear();
                
            } else if (bytes_read == 0) {
                // Connection closed
                std::cout << "\nConnection closed by server" << std::endl;
                break;
            } else {
                // Would block or error
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No data available, wait a bit
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                } else {
                    std::cerr << "Error reading from socket" << std::endl;
                    break;
                }
            }
        }
        
        // Print final order book
        std::cout << "\n========================================" << std::endl;
        std::cout << "Final Order Book" << std::endl;
        std::cout << "========================================" << std::endl;
        order_book_.print_all_spreads();
        
        // Print statistics
        auto stats = parser_.get_recovery_stats();
        std::cout << "\n========================================" << std::endl;
        std::cout << "Statistics" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Messages processed: " << message_count << std::endl;
        std::cout << "Parser recoveries: " << stats.recovery_count << std::endl;
        std::cout << "Bytes skipped: " << stats.bytes_skipped << std::endl;
        std::cout << std::endl;
    }
    
private:
    int socket_fd_;
    net::ReceiveBuffer buffer_;
    parser::FSMFixParser parser_;
    OrderBook order_book_;
};

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 9999;
    
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = std::atoi(argv[2]);
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "FeedHandler Demo - Final Assembly" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nConnecting to " << host << ":" << port << "..." << std::endl;
    
    FeedHandler handler;
    
    if (!handler.connect(host, port)) {
        std::cerr << "\nFailed to connect. Make sure mock server is running:" << std::endl;
        std::cerr << "  ./mock_fix_server " << port << std::endl;
        return 1;
    }
    
    handler.run();
    
    return 0;
}
