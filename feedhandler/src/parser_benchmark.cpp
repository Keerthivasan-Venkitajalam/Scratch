#include <iostream>
#include <vector>
#include <iomanip>
#include "parser/naive_fix_parser.hpp"
#include "parser/stringview_fix_parser.hpp"
#include "parser/optimized_fix_parser.hpp"
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

void test_stringview_parser() {
    std::cout << "=== String_view Parser Test ===" << std::endl;
    
    // Test message with all required fields
    std::string test_buffer = "8=FIX.4.4|9=79|35=D|55=MSFT|44=123.4500|38=1000|54=1|52=20240131-12:34:56|10=020|";
    
    std::cout << "Input message: " << test_buffer << std::endl;
    
    auto tick = parser::StringViewFixParser::parse_message(test_buffer);
    
    std::cout << "Parsed tick:" << std::endl;
    std::cout << "  Symbol: " << tick.symbol << std::endl;
    std::cout << "  Price: $" << std::fixed << std::setprecision(4) 
              << common::price_to_double(tick.price) << std::endl;
    std::cout << "  Quantity: " << tick.qty << std::endl;
    std::cout << "  Side: " << tick.side << std::endl;
    std::cout << "  Valid: " << (tick.is_valid() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
}

void test_optimized_parser() {
    std::cout << "=== Optimized Parser Test ===" << std::endl;
    
    // Test message with all required fields
    std::string test_buffer = "8=FIX.4.4|9=79|35=D|55=MSFT|44=123.4500|38=1000|54=1|52=20240131-12:34:56|10=020|";
    
    std::cout << "Input message: " << test_buffer << std::endl;
    
    auto tick = parser::OptimizedFixParser::parse_message(test_buffer);
    
    std::cout << "Parsed tick:" << std::endl;
    std::cout << "  Symbol: " << tick.symbol << std::endl;
    std::cout << "  Price: $" << std::fixed << std::setprecision(4) 
              << common::price_to_double(tick.price) << std::endl;
    std::cout << "  Quantity: " << tick.qty << std::endl;
    std::cout << "  Side: " << tick.side << std::endl;
    std::cout << "  Valid: " << (tick.is_valid() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
}

void run_benchmarks() {
    std::cout << "=== Performance Benchmarks ===" << std::endl;
    
    // Test different message counts
    std::vector<size_t> test_sizes = {1000, 10000, 100000, 1000000};
    
    for (size_t size : test_sizes) {
        std::cout << "\n--- Testing " << size << " messages ---" << std::endl;
        
        // Naive parser benchmark
        uint64_t naive_time = parser::NaiveFixParser::benchmark_parsing(size);
        std::cout << std::endl;
        
        // String_view parser benchmark
        uint64_t stringview_time = parser::StringViewFixParser::benchmark_parsing(size);
        std::cout << std::endl;
        
        // Optimized parser benchmark
        uint64_t optimized_time = parser::OptimizedFixParser::benchmark_parsing(size);
        
        // Calculate improvements
        double stringview_improvement = static_cast<double>(naive_time) / stringview_time;
        double optimized_improvement = static_cast<double>(naive_time) / optimized_time;
        double fast_number_improvement = static_cast<double>(stringview_time) / optimized_time;
        
        std::cout << "\nPerformance Summary:" << std::endl;
        std::cout << "  String_view vs Naive: " << std::fixed << std::setprecision(2) 
                  << stringview_improvement << "x faster" << std::endl;
        std::cout << "  Optimized vs Naive: " << std::fixed << std::setprecision(2) 
                  << optimized_improvement << "x faster" << std::endl;
        std::cout << "  Optimized vs String_view: " << std::fixed << std::setprecision(2) 
                  << fast_number_improvement << "x faster" << std::endl;
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "FeedHandler Parser Comparison Benchmark" << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << std::endl;
    
    test_single_message();
    test_stringview_parser();
    test_optimized_parser();
    run_benchmarks();
    
    return 0;
}