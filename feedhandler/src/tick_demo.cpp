#include <iostream>
#include <iomanip>
#include "common/tick.hpp"

using namespace feedhandler::common;

int main() {
    std::cout << "=== Tick Struct Demo ===" << std::endl;
    
    // Simulate FIX message buffer (normally from socket)
    const char* fix_buffer = "MSFT";  // Symbol from parsed FIX message
    
    // Create tick with zero-copy symbol reference
    Tick tick1(std::string_view(fix_buffer, 4), 1234567, 1000, 'B');
    
    std::cout << "Tick 1:" << std::endl;
    std::cout << "  Symbol: " << tick1.symbol << std::endl;
    std::cout << "  Price: $" << std::fixed << std::setprecision(4) 
              << price_to_double(tick1.price) << std::endl;
    std::cout << "  Quantity: " << tick1.qty << std::endl;
    std::cout << "  Side: " << tick1.side << std::endl;
    std::cout << "  Valid: " << (tick1.is_valid() ? "Yes" : "No") << std::endl;
    std::cout << "  Timestamp: " << tick1.timestamp << " ns" << std::endl;
    
    // Create another tick
    const char* btc_buffer = "BTC-USD";
    Tick tick2(std::string_view(btc_buffer, 7), 
               double_to_price(45123.75), 
               50, 
               fix_side_to_char(2));  // FIX side 2 = Sell
    
    std::cout << "\nTick 2:" << std::endl;
    std::cout << "  Symbol: " << tick2.symbol << std::endl;
    std::cout << "  Price: $" << std::fixed << std::setprecision(2) 
              << price_to_double(tick2.price) << std::endl;
    std::cout << "  Quantity: " << tick2.qty << std::endl;
    std::cout << "  Side: " << tick2.side << std::endl;
    std::cout << "  Valid: " << (tick2.is_valid() ? "Yes" : "No") << std::endl;
    
    // Demonstrate struct size
    std::cout << "\n=== Memory Layout ===" << std::endl;
    std::cout << "Tick struct size: " << sizeof(Tick) << " bytes" << std::endl;
    std::cout << "string_view size: " << sizeof(std::string_view) << " bytes" << std::endl;
    std::cout << "int64_t size: " << sizeof(int64_t) << " bytes" << std::endl;
    std::cout << "int32_t size: " << sizeof(int32_t) << " bytes" << std::endl;
    std::cout << "char size: " << sizeof(char) << " bytes" << std::endl;
    std::cout << "uint64_t size: " << sizeof(uint64_t) << " bytes" << std::endl;
    
    // Test invalid tick
    Tick invalid_tick;
    std::cout << "\nInvalid tick valid: " << (invalid_tick.is_valid() ? "Yes" : "No") << std::endl;
    
    return 0;
}