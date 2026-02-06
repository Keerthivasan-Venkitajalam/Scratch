#include <iostream>
#include <iomanip>
#include "parser/repeating_group_parser.hpp"
#include "common/tick.hpp"

using namespace feedhandler;

void test_single_entry() {
    std::cout << "=== Test: Single Entry (No Repeating Group) ===" << std::endl;
    
    std::string message = "8=FIX.4.4|35=D|55=AAPL|44=150.25|38=500|54=1|";
    
    auto ticks = parser::RepeatingGroupParser::parse_repeating_groups(message);
    
    std::cout << "Input: " << message << std::endl;
    std::cout << "Parsed " << ticks.size() << " tick(s):" << std::endl;
    
    for (const auto& tick : ticks) {
        std::cout << "  Symbol: " << tick.symbol 
                  << ", Price: $" << std::fixed << std::setprecision(2) << common::price_to_double(tick.price)
                  << ", Qty: " << tick.qty
                  << ", Side: " << tick.side << std::endl;
    }
    std::cout << std::endl;
}

void test_market_data_snapshot() {
    std::cout << "=== Test: Market Data Snapshot (3 Price Levels) ===" << std::endl;
    
    // Market data snapshot with 3 entries: 2 bids, 1 offer
    std::string message = "8=FIX.4.4|35=W|55=MSFT|268=3|"
                         "269=0|270=100.50|271=1000|"  // Bid
                         "269=0|270=100.25|271=500|"   // Bid
                         "269=1|270=100.75|271=750|";  // Offer
    
    auto ticks = parser::RepeatingGroupParser::parse_repeating_groups(message);
    
    std::cout << "Input: Market data with 3 price levels" << std::endl;
    std::cout << "Parsed " << ticks.size() << " tick(s):" << std::endl;
    
    for (size_t i = 0; i < ticks.size(); ++i) {
        const auto& tick = ticks[i];
        std::string side_str = (tick.side == 'B') ? "Bid" : (tick.side == 'S') ? "Offer" : "Trade";
        std::cout << "  " << (i+1) << ". " << tick.symbol 
                  << " " << side_str
                  << " $" << std::fixed << std::setprecision(2) << common::price_to_double(tick.price)
                  << " x " << tick.qty << std::endl;
    }
    std::cout << std::endl;
}

void test_order_book_levels() {
    std::cout << "=== Test: Full Order Book (5 Bids + 5 Offers) ===" << std::endl;
    
    // Full order book with 10 levels
    std::string message = "8=FIX.4.4|35=W|55=BTC-USD|268=10|"
                         // 5 Bid levels
                         "269=0|270=45100.00|271=1000|"
                         "269=0|270=45099.50|271=750|"
                         "269=0|270=45099.00|271=500|"
                         "269=0|270=45098.50|271=250|"
                         "269=0|270=45098.00|271=100|"
                         // 5 Offer levels
                         "269=1|270=45100.50|271=900|"
                         "269=1|270=45101.00|271=800|"
                         "269=1|270=45101.50|271=600|"
                         "269=1|270=45102.00|271=400|"
                         "269=1|270=45102.50|271=200|";
    
    auto ticks = parser::RepeatingGroupParser::parse_repeating_groups(message);
    
    std::cout << "Input: Full order book with 10 levels" << std::endl;
    std::cout << "Parsed " << ticks.size() << " tick(s):" << std::endl;
    
    // Separate bids and offers
    std::vector<common::Tick> bids, offers;
    for (const auto& tick : ticks) {
        if (tick.side == 'B') bids.push_back(tick);
        else if (tick.side == 'S') offers.push_back(tick);
    }
    
    std::cout << "\nBids (" << bids.size() << "):" << std::endl;
    for (const auto& tick : bids) {
        std::cout << "  $" << std::fixed << std::setprecision(2) << common::price_to_double(tick.price)
                  << " x " << tick.qty << std::endl;
    }
    
    std::cout << "\nOffers (" << offers.size() << "):" << std::endl;
    for (const auto& tick : offers) {
        std::cout << "  $" << std::fixed << std::setprecision(2) << common::price_to_double(tick.price)
                  << " x " << tick.qty << std::endl;
    }
    std::cout << std::endl;
}

void test_multiple_messages() {
    std::cout << "=== Test: Multiple Messages in Buffer ===" << std::endl;
    
    std::string buffer = 
        "8=FIX.4.4|35=W|55=AAPL|268=2|269=0|270=150.00|271=1000|269=1|270=150.25|271=500|\n"
        "8=FIX.4.4|35=W|55=GOOGL|268=2|269=0|270=2750.00|271=100|269=1|270=2751.00|271=75|\n"
        "8=FIX.4.4|35=W|55=TSLA|268=3|269=0|270=245.50|271=750|269=0|270=245.25|271=500|269=1|270=245.75|271=250|";
    
    auto ticks = parser::RepeatingGroupParser::parse_buffer_with_repeating_groups(buffer);
    
    std::cout << "Input: 3 messages with repeating groups" << std::endl;
    std::cout << "Total ticks parsed: " << ticks.size() << std::endl;
    
    // Group by symbol
    std::string current_symbol;
    int count = 0;
    for (const auto& tick : ticks) {
        if (tick.symbol != current_symbol) {
            if (!current_symbol.empty()) std::cout << std::endl;
            current_symbol = std::string(tick.symbol);
            count = 0;
            std::cout << current_symbol << ":" << std::endl;
        }
        ++count;
        std::string side_str = (tick.side == 'B') ? "Bid" : "Offer";
        std::cout << "  " << count << ". " << side_str
                  << " $" << std::fixed << std::setprecision(2) << common::price_to_double(tick.price)
                  << " x " << tick.qty << std::endl;
    }
    std::cout << std::endl;
}

void run_benchmarks() {
    std::cout << "=== Performance Benchmarks ===" << std::endl;
    
    // Test different configurations
    std::vector<std::pair<size_t, size_t>> configs = {
        {1000, 1},    // 1K messages, 1 entry each
        {1000, 5},    // 1K messages, 5 entries each
        {1000, 10},   // 1K messages, 10 entries each
        {10000, 5},   // 10K messages, 5 entries each
        {100000, 3},  // 100K messages, 3 entries each
    };
    
    for (const auto& [msg_count, entries] : configs) {
        std::cout << "\n--- Configuration: " << msg_count << " messages, " 
                  << entries << " entries each ---" << std::endl;
        parser::RepeatingGroupParser::benchmark_repeating_groups(msg_count, entries);
    }
}

int main() {
    std::cout << "Repeating Group Parser Test Suite" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << std::endl;
    
    test_single_entry();
    test_market_data_snapshot();
    test_order_book_levels();
    test_multiple_messages();
    run_benchmarks();
    
    return 0;
}