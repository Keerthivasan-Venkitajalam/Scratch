#include <iostream>
#include <vector>
#include <iomanip>
#include <string>
#include "parser/fsm_fix_parser.hpp"
#include "common/tick.hpp"

using namespace feedhandler;

void test_complete_message() {
    std::cout << "=== Test 1: Complete Message ===" << std::endl;
    
    parser::FSMFixParser parser;
    std::vector<common::Tick> ticks;
    
    std::string message = "8=FIX.4.4|35=D|55=AAPL|44=150.25|38=500|54=1|10=123|\n";
    
    std::cout << "Input: " << message << std::endl;
    
    size_t consumed = parser.parse(message.data(), message.size(), ticks);
    
    std::cout << "Consumed: " << consumed << " bytes" << std::endl;
    std::cout << "Ticks parsed: " << ticks.size() << std::endl;
    
    if (!ticks.empty()) {
        const auto& tick = ticks[0];
        std::cout << "Tick: " << tick.symbol 
                  << " $" << std::fixed << std::setprecision(2) << common::price_to_double(tick.price)
                  << " qty:" << tick.qty 
                  << " side:" << tick.side << std::endl;
    }
    std::cout << std::endl;
}

void test_fragmented_message() {
    std::cout << "=== Test 2: Fragmented Message (Streaming) ===" << std::endl;
    
    parser::FSMFixParser parser;
    std::vector<common::Tick> ticks;
    
    // Simulate TCP fragmentation - message split across multiple recv() calls
    std::vector<std::string> fragments = {
        "8=FIX.4.4|35=D|55=GO",
        "OGL|44=2750.",
        "80|38=100|54=2|10=",
        "456|\n"
    };
    
    std::cout << "Simulating fragmented TCP stream..." << std::endl;
    
    for (size_t i = 0; i < fragments.size(); ++i) {
        std::cout << "Fragment " << (i+1) << ": \"" << fragments[i] << "\"" << std::endl;
        
        size_t consumed = parser.parse(fragments[i].data(), fragments[i].size(), ticks);
        
        std::cout << "  Consumed: " << consumed << " bytes" << std::endl;
        std::cout << "  Parser state: " << (parser.is_parsing() ? "parsing" : "idle") << std::endl;
        std::cout << "  Ticks so far: " << ticks.size() << std::endl;
    }
    
    std::cout << "\nFinal result:" << std::endl;
    std::cout << "Total ticks: " << ticks.size() << std::endl;
    
    if (!ticks.empty()) {
        const auto& tick = ticks[0];
        std::cout << "Tick: " << tick.symbol 
                  << " $" << std::fixed << std::setprecision(2) << common::price_to_double(tick.price)
                  << " qty:" << tick.qty 
                  << " side:" << tick.side << std::endl;
    }
    std::cout << std::endl;
}

void test_multiple_messages_in_buffer() {
    std::cout << "=== Test 3: Multiple Messages in Single Buffer ===" << std::endl;
    
    parser::FSMFixParser parser;
    std::vector<common::Tick> ticks;
    
    std::string buffer = 
        "8=FIX.4.4|35=D|55=AAPL|44=150.25|38=500|54=1|10=123|\n"
        "8=FIX.4.4|35=D|55=MSFT|44=123.45|38=1000|54=2|10=456|\n"
        "8=FIX.4.4|35=D|55=TSLA|44=245.67|38=750|54=1|10=789|\n";
    
    std::cout << "Buffer contains 3 messages" << std::endl;
    
    size_t consumed = parser.parse(buffer.data(), buffer.size(), ticks);
    
    std::cout << "Consumed: " << consumed << " bytes" << std::endl;
    std::cout << "Ticks parsed: " << ticks.size() << std::endl;
    
    for (size_t i = 0; i < ticks.size(); ++i) {
        const auto& tick = ticks[i];
        std::cout << "  " << (i+1) << ". " << tick.symbol 
                  << " $" << std::fixed << std::setprecision(2) << common::price_to_double(tick.price)
                  << " qty:" << tick.qty 
                  << " side:" << tick.side << std::endl;
    }
    std::cout << std::endl;
}

void test_partial_message_resume() {
    std::cout << "=== Test 4: Partial Message with Resume ===" << std::endl;
    
    parser::FSMFixParser parser;
    std::vector<common::Tick> ticks;
    
    // First chunk - incomplete message
    std::string chunk1 = "8=FIX.4.4|35=D|55=BTC-USD|44=45";
    std::cout << "Chunk 1: \"" << chunk1 << "\"" << std::endl;
    
    size_t consumed1 = parser.parse(chunk1.data(), chunk1.size(), ticks);
    std::cout << "  Consumed: " << consumed1 << " bytes, Ticks: " << ticks.size() 
              << ", Parsing: " << (parser.is_parsing() ? "yes" : "no") << std::endl;
    
    // Second chunk - continuation
    std::string chunk2 = "123.75|38=50|54=2|10=999|\n";
    std::cout << "Chunk 2: \"" << chunk2 << "\"" << std::endl;
    
    size_t consumed2 = parser.parse(chunk2.data(), chunk2.size(), ticks);
    std::cout << "  Consumed: " << consumed2 << " bytes, Ticks: " << ticks.size() 
              << ", Parsing: " << (parser.is_parsing() ? "yes" : "no") << std::endl;
    
    std::cout << "\nFinal result:" << std::endl;
    if (!ticks.empty()) {
        const auto& tick = ticks[0];
        std::cout << "Tick: " << tick.symbol 
                  << " $" << std::fixed << std::setprecision(2) << common::price_to_double(tick.price)
                  << " qty:" << tick.qty 
                  << " side:" << tick.side << std::endl;
    }
    std::cout << std::endl;
}

void run_benchmark() {
    std::cout << "=== Performance Benchmark ===" << std::endl;
    
    std::vector<size_t> test_sizes = {1000, 10000, 100000, 1000000};
    
    for (size_t size : test_sizes) {
        parser::FSMFixParser::benchmark_parsing(size);
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "FSM FIX Parser Test Suite" << std::endl;
    std::cout << "=========================" << std::endl;
    std::cout << std::endl;
    
    test_complete_message();
    test_fragmented_message();
    test_multiple_messages_in_buffer();
    test_partial_message_resume();
    run_benchmark();
    
    return 0;
}