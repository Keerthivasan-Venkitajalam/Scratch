#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include "net/tcp_client.hpp"
#include "net/event_loop.hpp"

int main() {
    std::cout << "FeedHandler Boot OK" << std::endl;
    
    feedhandler::net::TcpClient client;
    
    // Test connection to localhost:8080
    if (client.connect("localhost", 8080)) {
        std::cout << "Connected to server" << std::endl;
        
        // Set socket to non-blocking mode (Day 3)
        std::cout << "Socket set to non-blocking mode" << std::endl;
        
        // Create event loop (Day 3)
        feedhandler::net::EventLoop loop;
        
        std::cout << "Enter messages to send (type 'quit' to exit):" << std::endl;
        std::cout << "Program will not block on recv() — demonstrates non-blocking with select()" << std::endl;
        
        std::string input;
        while (std::getline(std::cin, input) && input != "quit") {
            if (client.send(input + "\n")) {
                // Use non-blocking receive
                std::string response = client.recv();
                if (!response.empty()) {
                    std::cout << "Received: " << response;
                }
            }
        }
    } else {
        std::cout << "Failed to connect. Make sure to run 'nc -l 8080' in another terminal." << std::endl;
    }
    
    return 0;
}

