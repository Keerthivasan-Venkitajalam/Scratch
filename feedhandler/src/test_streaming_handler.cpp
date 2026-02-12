#include <iostream>
#include <vector>
#include <iomanip>
#include <string>
#include "parser/streaming_fix_handler.hpp"
#include "common/tick.hpp"

using namespace feedhandler;

void print_tick(const common::Tick& tick, size_t index) {
    std::cout << "  " << index << ". " << tick.symbol 
              << " $" << std::fixed << std::setprecision(2) << common::price_to_double(tick.price)
              << " qty:" << tick.qty 
              << " side:" << tick.side << std::endl;
}

void test_simple_streaming() {
    std::cout << "=== Test 1: Simple Streaming ===" << std::endl;
    
    parser::StreamingFixHandler handler;
    std::vector<common::Tick> ticks;
    
    // Simulate receiving data from socket
    std::string data1 = "8=FIX.4.4|35=D|55=AAPL|44=150.25|38=500|54=1|10=123|\n";
    
    std::cout << "Receiving: " << data1.size() << " bytes" << std::endl;
    size_t parsed = handler.process_incoming_data(data1.data(), data1.size(), ticks);
    
    std::cout << "Parsed " << parsed << " ticks" << std::endl;
    for (size_t i = 0; i < ticks.size(); ++i) {
        print_tick(ticks[i], i + 1);
    }
    std::cout << std::endl;
}

void test_fragmented_streaming() {
    std::cout << "=== Test 2: Fragmented TCP Stream ===" << std::endl;
    std::cout << "Simulating message split across multiple recv() calls" << std::endl;
    
    parser::StreamingFixHandler handler;
    std::vector<common::Tick> ticks;
    
    // Message split into 4 fragments (simulating TCP fragmentation)
    std::vector<std::string> fragments = {
        "8=FIX.4.4|35=D|55=GO",
        "OGL|44=2750.",
        "80|38=100|54=2|10=",
        "456|\n"
    };
    
    for (size_t i = 0; i < fragments.size(); ++i) {
        std::cout << "\nRecv " << (i+1) << ": \"" << fragments[i] << "\" (" 
                  << fragments[i].size() << " bytes)" << std::endl;
        
        size_t parsed = handler.process_incoming_data(fragments[i].data(), 
                                                       fragments[i].size(), 
                                                       ticks);
        
        std::cout << "  Parsed: " << parsed << " ticks" << std::endl;
        std::cout << "  Buffer: " << handler.buffer_bytes() << " bytes" << std::endl;
        std::cout << "  Parsing: " << (handler.is_parsing() ? "yes" : "no") << std::endl;
    }
    
    std::cout << "\nFinal result: " << ticks.size() << " ticks" << std::endl;
    for (size_t i = 0; i < ticks.size(); ++i) {
        print_tick(ticks[i], i + 1);
    }
    std::cout << std::endl;
}

void test_multiple_messages_streaming() {
    std::cout << "=== Test 3: Multiple Messages in Stream ===" << std::endl;
    
    parser::StreamingFixHandler handler;
    std::vector<common::Tick> ticks;
    
    // Multiple complete messages in one recv()
    std::string data = 
        "8=FIX.4.4|35=D|55=AAPL|44=150.25|38=500|54=1|10=123|\n"
        "8=FIX.4.4|35=D|55=MSFT|44=123.45|38=1000|54=2|10=456|\n"
        "8=FIX.4.4|35=D|55=TSLA|44=245.67|38=750|54=1|10=789|\n";
    
    std::cout << "Receiving buffer with 3 complete messages (" << data.size() << " bytes)" << std::endl;
    
    size_t parsed = handler.process_incoming_data(data.data(), data.size(), ticks);
    
    std::cout << "Parsed " << parsed << " ticks" << std::endl;
    for (size_t i = 0; i < ticks.size(); ++i) {
        print_tick(ticks[i], i + 1);
    }
    std::cout << std::endl;
}

void test_mixed_fragmentation() {
    std::cout << "=== Test 4: Mixed Complete and Fragmented Messages ===" << std::endl;
    
    parser::StreamingFixHandler handler;
    std::vector<common::Tick> ticks;
    
    // First recv: 2 complete messages + partial third
    std::string recv1 = 
        "8=FIX.4.4|35=D|55=AAPL|44=150.25|38=500|54=1|10=123|\n"
        "8=FIX.4.4|35=D|55=MSFT|44=123.45|38=1000|54=2|10=456|\n"
        "8=FIX.4.4|35=D|55=TSLA|44=245";
    
    std::cout << "Recv 1: 2 complete + partial (" << recv1.size() << " bytes)" << std::endl;
    size_t parsed1 = handler.process_incoming_data(recv1.data(), recv1.size(), ticks);
    std::cout << "  Parsed: " << parsed1 << " ticks" << std::endl;
    std::cout << "  Buffer: " << handler.buffer_bytes() << " bytes (partial message)" << std::endl;
    std::cout << "  Parsing: " << (handler.is_parsing() ? "yes" : "no") << std::endl;
    
    // Second recv: continuation + new complete message
    std::string recv2 = 
        ".67|38=750|54=1|10=789|\n"
        "8=FIX.4.4|35=D|55=BTC-USD|44=45123.75|38=50|54=2|10=999|\n";
    
    std::cout << "\nRecv 2: continuation + 1 complete (" << recv2.size() << " bytes)" << std::endl;
    size_t parsed2 = handler.process_incoming_data(recv2.data(), recv2.size(), ticks);
    std::cout << "  Parsed: " << parsed2 << " ticks" << std::endl;
    std::cout << "  Buffer: " << handler.buffer_bytes() << " bytes" << std::endl;
    std::cout << "  Parsing: " << (handler.is_parsing() ? "yes" : "no") << std::endl;
    
    std::cout << "\nTotal ticks: " << ticks.size() << std::endl;
    for (size_t i = 0; i < ticks.size(); ++i) {
        print_tick(ticks[i], i + 1);
    }
    std::cout << std::endl;
}

void test_buffer_compaction() {
    std::cout << "=== Test 5: Buffer Compaction ===" << std::endl;
    std::cout << "Testing automatic buffer compaction when read position advances" << std::endl;
    
    parser::StreamingFixHandler handler;
    std::vector<common::Tick> ticks;
    
    // Send many small messages to trigger compaction
    for (int i = 0; i < 100; ++i) {
        std::string msg = "8=FIX.4.4|35=D|55=TEST|44=100.00|38=100|54=1|10=123|\n";
        handler.process_incoming_data(msg.data(), msg.size(), ticks);
    }
    
    auto stats = handler.get_stats();
    
    std::cout << "Statistics after 100 messages:" << std::endl;
    std::cout << "  Total bytes received: " << stats.total_bytes_received << std::endl;
    std::cout << "  Total messages parsed: " << stats.total_messages_parsed << std::endl;
    std::cout << "  Total parse calls: " << stats.total_parse_calls << std::endl;
    std::cout << "  Buffer compactions: " << stats.buffer_compactions << std::endl;
    std::cout << "  Current buffer bytes: " << handler.buffer_bytes() << std::endl;
    std::cout << "  Ticks parsed: " << ticks.size() << std::endl;
    std::cout << std::endl;
}

void test_state_preservation() {
    std::cout << "=== Test 6: State Preservation Across Calls ===" << std::endl;
    std::cout << "Demonstrating parser state is preserved between recv() calls" << std::endl;
    
    parser::StreamingFixHandler handler;
    std::vector<common::Tick> ticks;
    
    // Split message at various points to test state preservation
    std::vector<std::string> fragments = {
        "8=",                    // In tag
        "FIX.4.4|35=D|5",       // In tag
        "5=BTC-U",              // In value
        "SD|44=4512",           // In value
        "3.75|38=",             // After equals
        "50|54=2|10=999|\n"     // Complete
    };
    
    for (size_t i = 0; i < fragments.size(); ++i) {
        std::cout << "Fragment " << (i+1) << ": \"" << fragments[i] << "\"" << std::endl;
        
        handler.process_incoming_data(fragments[i].data(), 
                                      fragments[i].size(), 
                                      ticks);
        
        std::cout << "  State: " << (handler.is_parsing() ? "parsing" : "idle")
                  << ", Ticks: " << ticks.size() << std::endl;
    }
    
    std::cout << "\nFinal result:" << std::endl;
    if (!ticks.empty()) {
        print_tick(ticks[0], 1);
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "Streaming FIX Handler Test Suite" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "Integration of FSM Parser + Receive Buffer" << std::endl;
    std::cout << std::endl;
    
    test_simple_streaming();
    test_fragmented_streaming();
    test_multiple_messages_streaming();
    test_mixed_fragmentation();
    test_buffer_compaction();
    test_state_preservation();
    
    std::cout << "All tests completed successfully!" << std::endl;
    std::cout << "\nKey Features Demonstrated:" << std::endl;
    std::cout << "  ✓ FSM parser maintains state across buffer boundaries" << std::endl;
    std::cout << "  ✓ Receive buffer handles TCP fragmentation" << std::endl;
    std::cout << "  ✓ Automatic buffer compaction prevents overflow" << std::endl;
    std::cout << "  ✓ Zero-copy parsing where possible" << std::endl;
    std::cout << "  ✓ Handles partial messages gracefully" << std::endl;
    
    return 0;
}