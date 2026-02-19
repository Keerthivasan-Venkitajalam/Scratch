// Test program for garbage recovery in FSM parser
// Demonstrates how parser recovers from corrupted data by scanning for "8=FIX"

#include "parser/fsm_fix_parser.hpp"
#include "common/tick.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace feedhandler;

void print_separator() {
    std::cout << "========================================" << std::endl;
}

void test_clean_messages() {
    print_separator();
    std::cout << "Test 1: Clean Messages (No Errors)" << std::endl;
    print_separator();
    
    parser::FSMFixParser parser;
    parser.set_garbage_recovery(true);
    
    std::string data = 
        "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.25|38=500|54=1|52=20240131-12:34:56|10=020|\n"
        "8=FIX.4.4|9=79|35=D|55=GOOGL|44=2800.50|38=100|54=2|52=20240131-12:34:57|10=021|\n";
    
    std::vector<common::Tick> ticks;
    size_t consumed = parser.parse(data.data(), data.size(), ticks);
    
    std::cout << "Consumed: " << consumed << " bytes" << std::endl;
    std::cout << "Parsed: " << ticks.size() << " ticks" << std::endl;
    
    auto stats = parser.get_recovery_stats();
    std::cout << "Errors: " << stats.error_count << std::endl;
    std::cout << "Recoveries: " << stats.recovery_count << std::endl;
    std::cout << "Bytes skipped: " << stats.bytes_skipped << std::endl;
    std::cout << std::endl;
}

void test_garbage_at_start() {
    print_separator();
    std::cout << "Test 2: Garbage at Start" << std::endl;
    print_separator();
    
    parser::FSMFixParser parser;
    parser.set_garbage_recovery(true);
    
    // Garbage data followed by valid message
    std::string data = 
        "GARBAGE_DATA_HERE_CORRUPT!!!"
        "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.25|38=500|54=1|52=20240131-12:34:56|10=020|\n";
    
    std::vector<common::Tick> ticks;
    
    // First pass - will encounter garbage
    size_t offset = 0;
    while (offset < data.size()) {
        // Check if we need recovery
        if (!parser.is_fix_message_start(data.data() + offset, data.size() - offset) && 
            parser.is_garbage_recovery_enabled()) {
            
            // Attempt recovery
            size_t skip = parser.attempt_garbage_recovery(data.data() + offset, data.size() - offset);
            std::cout << "Skipping " << skip << " bytes of garbage" << std::endl;
            offset += skip;
            parser.reset();  // Reset parser state after recovery
            continue;
        }
        
        // Parse normally
        size_t consumed = parser.parse(data.data() + offset, data.size() - offset, ticks);
        offset += consumed;
        
        if (consumed == 0) break;  // No progress
    }
    
    std::cout << "Parsed: " << ticks.size() << " ticks" << std::endl;
    
    auto stats = parser.get_recovery_stats();
    std::cout << "Recoveries: " << stats.recovery_count << std::endl;
    std::cout << "Bytes skipped: " << stats.bytes_skipped << std::endl;
    std::cout << std::endl;
}

void test_garbage_between_messages() {
    print_separator();
    std::cout << "Test 3: Garbage Between Messages" << std::endl;
    print_separator();
    
    parser::FSMFixParser parser;
    parser.set_garbage_recovery(true);
    
    // Valid message, garbage, valid message
    std::string data = 
        "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.25|38=500|54=1|52=20240131-12:34:56|10=020|\n"
        "CORRUPT_DATA_BETWEEN_MESSAGES!!!"
        "8=FIX.4.4|9=79|35=D|55=GOOGL|44=2800.50|38=100|54=2|52=20240131-12:34:57|10=021|\n";
    
    std::vector<common::Tick> ticks;
    size_t offset = 0;
    
    while (offset < data.size()) {
        // Check if we're at a valid message start
        if (!parser.is_fix_message_start(data.data() + offset, data.size() - offset) && 
            parser.is_garbage_recovery_enabled() && 
            !parser.is_parsing()) {
            
            // Attempt recovery
            size_t skip = parser.attempt_garbage_recovery(data.data() + offset, data.size() - offset);
            std::cout << "Skipping " << skip << " bytes of garbage" << std::endl;
            offset += skip;
            parser.reset();
            continue;
        }
        
        // Parse normally
        size_t consumed = parser.parse(data.data() + offset, data.size() - offset, ticks);
        offset += consumed;
        
        if (consumed == 0) break;
    }
    
    std::cout << "Parsed: " << ticks.size() << " ticks" << std::endl;
    
    for (size_t i = 0; i < ticks.size(); ++i) {
        std::cout << "  Tick " << (i+1) << ": " << std::string(ticks[i].symbol.data(), ticks[i].symbol.size())
                  << " @ " << common::price_to_double(ticks[i].price) << std::endl;
    }
    
    auto stats = parser.get_recovery_stats();
    std::cout << "Recoveries: " << stats.recovery_count << std::endl;
    std::cout << "Bytes skipped: " << stats.bytes_skipped << std::endl;
    std::cout << std::endl;
}

void test_partial_fix_pattern() {
    print_separator();
    std::cout << "Test 4: Partial FIX Pattern in Garbage" << std::endl;
    print_separator();
    
    parser::FSMFixParser parser;
    parser.set_garbage_recovery(true);
    
    // Garbage with partial "8=FI" pattern, then real message
    std::string data = 
        "GARBAGE_8=FI_NOT_COMPLETE_8=F_ALSO_NOT_"
        "8=FIX.4.4|9=79|35=D|55=TSLA|44=245.75|38=750|54=1|52=20240131-12:34:58|10=022|\n";
    
    std::vector<common::Tick> ticks;
    size_t offset = 0;
    
    while (offset < data.size()) {
        if (!parser.is_fix_message_start(data.data() + offset, data.size() - offset) && 
            parser.is_garbage_recovery_enabled() && 
            !parser.is_parsing()) {
            
            size_t skip = parser.attempt_garbage_recovery(data.data() + offset, data.size() - offset);
            std::cout << "Skipping " << skip << " bytes (partial patterns ignored)" << std::endl;
            offset += skip;
            parser.reset();
            continue;
        }
        
        size_t consumed = parser.parse(data.data() + offset, data.size() - offset, ticks);
        offset += consumed;
        
        if (consumed == 0) break;
    }
    
    std::cout << "Parsed: " << ticks.size() << " ticks" << std::endl;
    
    if (!ticks.empty()) {
        std::cout << "  Symbol: " << std::string(ticks[0].symbol.data(), ticks[0].symbol.size()) << std::endl;
        std::cout << "  Price: " << common::price_to_double(ticks[0].price) << std::endl;
    }
    
    auto stats = parser.get_recovery_stats();
    std::cout << "Recoveries: " << stats.recovery_count << std::endl;
    std::cout << "Bytes skipped: " << stats.bytes_skipped << std::endl;
    std::cout << std::endl;
}

void test_recovery_disabled() {
    print_separator();
    std::cout << "Test 5: Recovery Disabled (Baseline)" << std::endl;
    print_separator();
    
    parser::FSMFixParser parser;
    parser.set_garbage_recovery(false);  // Disabled
    
    std::string data = 
        "GARBAGE"
        "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.25|38=500|54=1|52=20240131-12:34:56|10=020|\n";
    
    std::vector<common::Tick> ticks;
    size_t consumed = parser.parse(data.data(), data.size(), ticks);
    
    std::cout << "Consumed: " << consumed << " bytes" << std::endl;
    std::cout << "Parsed: " << ticks.size() << " ticks (expected 0 - garbage not handled)" << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "\n";
    print_separator();
    std::cout << "FSM Parser Garbage Recovery Tests" << std::endl;
    print_separator();
    std::cout << "\n";
    
    test_clean_messages();
    test_garbage_at_start();
    test_garbage_between_messages();
    test_partial_fix_pattern();
    test_recovery_disabled();
    
    print_separator();
    std::cout << "All tests complete!" << std::endl;
    print_separator();
    std::cout << "\n";
    
    return 0;
}
