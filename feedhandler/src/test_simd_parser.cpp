#include "parser/simd_fix_parser.hpp"
#include <iostream>
#include <chrono>
#include <vector>

using namespace feedhandler;

void test_simd_delimiter_search() {
    std::cout << "Testing SIMD delimiter search..." << std::endl;
    
    parser::SIMDFixParser parser;
    std::string test_data = "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.25|38=500|54=1|10=020|";
    
    auto positions = parser.find_delimiters_simd(test_data.c_str(), test_data.size(), '|');
    
    std::cout << "Found " << positions.size() << " delimiters at positions: ";
    for (size_t pos : positions) {
        std::cout << pos << " ";
    }
    std::cout << std::endl;
}

void test_simd_atoi() {
    std::cout << "\nTesting SIMD atoi..." << std::endl;
    
    parser::SIMDFixParser parser;
    
    std::vector<std::string> test_numbers = {
        "123", "0", "-456", "999999999", "42"
    };
    
    for (const auto& num_str : test_numbers) {
        int result = parser.simd_atoi(num_str);
        std::cout << "simd_atoi(\"" << num_str << "\") = " << result << std::endl;
    }
}

void test_simd_atof() {
    std::cout << "\nTesting SIMD atof..." << std::endl;
    
    parser::SIMDFixParser parser;
    
    std::vector<std::string> test_numbers = {
        "123.45", "0.0", "-456.789", "999.999", "42.0"
    };
    
    for (const auto& num_str : test_numbers) {
        double result = parser.simd_atof(num_str);
        std::cout << "simd_atof(\"" << num_str << "\") = " << result << std::endl;
    }
}

void benchmark_simd_vs_standard() {
    std::cout << "\nBenchmarking SIMD vs standard parsing..." << std::endl;
    
    const size_t message_count = 100000;
    
    // Benchmark SIMD parser
    auto simd_time = parser::SIMDFixParser::benchmark_parsing(message_count);
    
    std::cout << "SIMD Parser: " << message_count << " messages in " 
              << simd_time << " microseconds" << std::endl;
    std::cout << "Rate: " << (message_count * 1000000.0 / simd_time) 
              << " messages/second" << std::endl;
}

void test_full_message_parsing() {
    std::cout << "\nTesting full FIX message parsing..." << std::endl;
    
    parser::SIMDFixParser parser;
    std::vector<common::Tick> ticks;
    
    std::string fix_message = 
        "8=FIX.4.4\x01" "9=79\x01" "35=D\x01" "55=AAPL\x01" 
        "44=150.2500\x01" "38=500\x01" "54=1\x01" 
        "52=20240131-12:34:56\x01" "10=020\x01";
    
    size_t consumed = parser.parse(fix_message.c_str(), fix_message.size(), ticks);
    
    std::cout << "Parsed " << consumed << " bytes, extracted " 
              << ticks.size() << " ticks" << std::endl;
    
    for (const auto& tick : ticks) {
        std::cout << "Tick: " << tick.symbol << " @ " << common::price_to_double(tick.price)
                  << " qty=" << tick.qty << std::endl;
    }
}

int main() {
    std::cout << "=== SIMD FIX Parser Test Suite ===" << std::endl;
    
    try {
        test_simd_delimiter_search();
        test_simd_atoi();
        test_simd_atof();
        test_full_message_parsing();
        benchmark_simd_vs_standard();
        
        std::cout << "\n✅ All SIMD parser tests passed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}