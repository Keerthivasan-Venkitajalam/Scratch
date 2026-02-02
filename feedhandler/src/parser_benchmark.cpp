#include <iostream>
#include <vector>
#include <iomanip>
#include "parser/naive_fix_parser.hpp"
#include "common/tick.hpp"

using namespace feedhandler;

void test_single_message() {
    std::cout << "=== Single Message Test ===" << std::endl;
    
    // Test message with all required fields
    std::string test_message = "8=FIX.4.4|9=79|35=D|55=MSFT|44=123.4500|38=1000|54=1|52=20240131-12:34:56|10=020|";
    
    std::cout << "Input message: " << test_message << std::endl;
    
    auto tick = parser::NaiveFixParser::parse_message(test_message);
    
    std::cout << "Parsed tick:" << std::endl;
    std::cout << "  Symbol: " << tick.symbol << std::endl;
    std::cout << "  Price: $" << std::fixed << std::setprecision(4) 
              << common::price_to_double(tick.price) << std::endl;
    std::cout << "  Quantity: " << tick.qty << std::endl;
    std::cout << "  Side: " << tick.side << std::endl;
    std::cout << "  Valid: " << (tick.is_valid() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
}

void test_multiple_messages() {
    std::cout << "=== Multiple Messages Test ===" << std::endl;
    
    std::vector<std::string> messages = {
        "8=FIX.4.4|35=D|55=AAPL|44=150.25|38=500|54=1|",
        "8=FIX.4.4|35=D|55=GOOGL|44=2750.80|38=100|54=2|",
        "8=FIX.4.4|35=D|55=TSLA|44=245.67|38=750|54=1|",
        "8=FIX.4.4|35=D|55=BTC-USD|44=45123.75|38=50|54=2|"
    };
    
    auto ticks = parser::NaiveFixParser::parse_messages(messages);
    
    std::cout << "Parsed " << ticks.size() << " messages:" << std::endl;
    for (size_t i = 0; i < ticks.size(); ++i) {
        const auto& tick = ticks[i];
        std::cout << "  " << (i+1) << ". " << tick.symbol 
                  << " $" << std::fixed << std::setprecision(2) << common::price_to_double(tick.price)
                  << " qty:" << tick.qty 
                  << " side:" << tick.side
                  << " valid:" << (tick.is_valid() ? "Y" : "N") << std::endl;
    }
    std::cout << std::endl;
}

void run_benchmarks() {
    std::cout << "=== Performance Benchmarks ===" << std::endl;
    
    // Test different message counts
    std::vector<size_t> test_sizes = {1000, 10000, 100000, 1000000};
    
    for (size_t size : test_sizes) {
        parser::NaiveFixParser::benchmark_parsing(size);
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "FeedHandler Naive FIX Parser Benchmark" << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << std::endl;
    
    test_single_message();
    test_multiple_messages();
    run_benchmarks();
    
    return 0;
}