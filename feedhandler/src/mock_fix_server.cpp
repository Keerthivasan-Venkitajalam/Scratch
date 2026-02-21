// Mock FIX server that sends simulated market data
// Used for testing the complete FeedHandler pipeline

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

class MockFixServer {
public:
    MockFixServer(int port) : port_(port), server_fd_(-1), running_(false) {}
    
    ~MockFixServer() {
        stop();
    }
    
    bool start() {
        // Create socket
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }
        
        // Set socket options
        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "Failed to set socket options" << std::endl;
            close(server_fd_);
            return false;
        }
        
        // Bind to port
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);
        
        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Failed to bind to port " << port_ << std::endl;
            close(server_fd_);
            return false;
        }
        
        // Listen
        if (listen(server_fd_, 3) < 0) {
            std::cerr << "Failed to listen" << std::endl;
            close(server_fd_);
            return false;
        }
        
        running_ = true;
        std::cout << "Mock FIX server listening on port " << port_ << std::endl;
        return true;
    }
    
    void accept_and_send() {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            return;
        }
        
        std::cout << "Client connected from " 
                  << inet_ntoa(client_addr.sin_addr) << std::endl;
        
        // Send simulated market data
        send_market_data(client_fd);
        
        close(client_fd);
        std::cout << "Client disconnected" << std::endl;
    }
    
    void stop() {
        running_ = false;
        if (server_fd_ >= 0) {
            close(server_fd_);
            server_fd_ = -1;
        }
    }
    
private:
    void send_market_data(int client_fd) {
        // Simulated order book with bid/ask quotes
        std::vector<std::string> messages = {
            // Initial quotes
            "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.25|38=1000|54=1|52=20240131-12:00:00|10=001|\n",  // Bid
            "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.50|38=500|54=2|52=20240131-12:00:01|10=002|\n",   // Ask
            
            // Price updates
            "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.30|38=1500|54=1|52=20240131-12:00:02|10=003|\n",  // Bid up
            "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.45|38=800|54=2|52=20240131-12:00:03|10=004|\n",   // Ask down
            
            // More updates
            "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.35|38=2000|54=1|52=20240131-12:00:04|10=005|\n",  // Bid up
            "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.40|38=1200|54=2|52=20240131-12:00:05|10=006|\n",  // Ask down
            
            // Final quotes
            "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.38|38=1800|54=1|52=20240131-12:00:06|10=007|\n",  // Bid
            "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.42|38=1000|54=2|52=20240131-12:00:07|10=008|\n",  // Ask
        };
        
        std::cout << "\nSending " << messages.size() << " market data messages..." << std::endl;
        
        for (const auto& msg : messages) {
            // Send message
            ssize_t sent = send(client_fd, msg.c_str(), msg.length(), 0);
            if (sent < 0) {
                std::cerr << "Failed to send message" << std::endl;
                break;
            }
            
            std::cout << "Sent: " << msg.substr(0, msg.find('\n')) << std::endl;
            
            // Simulate realistic timing (100ms between messages)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "\nAll messages sent!" << std::endl;
    }
    
    int port_;
    int server_fd_;
    bool running_;
};

int main(int argc, char* argv[]) {
    int port = 9999;
    
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "Mock FIX Server" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    MockFixServer server(port);
    
    if (!server.start()) {
        return 1;
    }
    
    std::cout << "\nWaiting for client connection..." << std::endl;
    std::cout << "Connect with: ./feedhandler_demo localhost " << port << std::endl;
    std::cout << std::endl;
    
    // Accept one connection and send data
    server.accept_and_send();
    
    server.stop();
    
    std::cout << "\nServer stopped." << std::endl;
    return 0;
}
