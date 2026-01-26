#include <iostream>
#include <string>
#include "net/tcp_client.hpp"

int main() {
    std::cout << "FeedHandler Boot OK" << std::endl;
    
    feedhandler::net::TcpClient client;
    
    // Test connection to localhost:8080
    if (client.connect("localhost", 8080)) {
        std::cout << "Enter messages to send (type 'quit' to exit):" << std::endl;
        
        std::string input;
        while (std::getline(std::cin, input) && input != "quit") {
            if (client.send(input + "\n")) {
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

