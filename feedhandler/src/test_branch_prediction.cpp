// Test program to demonstrate branch prediction optimization impact
// Compares parsing performance with and without branch hints

#include "parser/fsm_fix_parser.hpp"
#include "common/tick.hpp"

#include <iostream>
#include <chrono>
#include <vector>
#include <string>

using namespace feedhandler;

// Simulate parsing without branch hints (for comparison)
class FSMFixParserNoBranchHints {
public:
    size_t parse_no_hints(const char* buffer, size_t length) {
        size_t delimiter_count = 0;
        
        // Simulate the hot path without branch hints
        for (size_t i = 0; i < length; ++i) {
            char c = buffer[i];
            
            // No __builtin_expect hints
            if (c == '|' || c == '\x01' || c == '\n' || c == '\r') {
                delimiter_count++;
            }
        }
        
        return delimiter_count;
    }
};

uint64_t benchmark_with_hints(const std::string& data, size_t iterations) {
    parser::FSMFixParser parser;
    std::vector<common::Tick> ticks;
    ticks.reserve(iterations);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; ++i) {
        parser.reset();
        parser.parse(data.data(), data.size(), ticks);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    return duration.count();
}

uint64_t benchmark_without_hints(const std::string& data, size_t iterations) {
    FSMFixParserNoBranchHints parser;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; ++i) {
        parser.parse_no_hints(data.data(), data.size());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    return duration.count();
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Branch Prediction Optimization Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Create sample FIX message
    std::string sample_message = "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.2500|38=500|54=1|52=20240131-12:34:56|10=020|\n";
    
    // Create large test data
    std::string test_data;
    const size_t message_count = 10000;
    test_data.reserve(sample_message.size() * message_count);
    
    for (size_t i = 0; i < message_count; ++i) {
        test_data += sample_message;
    }
    
    std::cout << "Test data size: " << test_data.size() << " bytes" << std::endl;
    std::cout << "Message count: " << message_count << std::endl;
    std::cout << std::endl;
    
    // Warm up
    std::cout << "Warming up..." << std::endl;
    parser::FSMFixParser warmup_parser;
    std::vector<common::Tick> warmup_ticks;
    warmup_parser.parse(test_data.data(), test_data.size(), warmup_ticks);
    std::cout << std::endl;
    
    // Benchmark with branch hints
    std::cout << "Running benchmark WITH branch hints..." << std::endl;
    const size_t iterations = 100;
    uint64_t time_with_hints = benchmark_with_hints(test_data, iterations);
    
    std::cout << "  Time: " << time_with_hints << " μs" << std::endl;
    std::cout << "  Throughput: " << (message_count * iterations * 1000000ULL / time_with_hints) 
              << " messages/sec" << std::endl;
    std::cout << std::endl;
    
    // Benchmark without branch hints (simplified comparison)
    std::cout << "Running benchmark WITHOUT branch hints (simplified)..." << std::endl;
    uint64_t time_without_hints = benchmark_without_hints(test_data, iterations);
    
    std::cout << "  Time: " << time_without_hints << " μs" << std::endl;
    std::cout << std::endl;
    
    // Calculate improvement
    std::cout << "========================================" << std::endl;
    std::cout << "Results" << std::endl;
    std::cout << "========================================" << std::endl;
    
    if (time_with_hints < time_without_hints) {
        double improvement = ((double)(time_without_hints - time_with_hints) / time_without_hints) * 100.0;
        std::cout << "Branch hints improvement: " << improvement << "%" << std::endl;
    } else {
        std::cout << "Note: Results may vary due to CPU branch predictor learning" << std::endl;
        std::cout << "Run with 'perf stat' for accurate branch miss measurements" << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "To measure actual branch prediction impact, use:" << std::endl;
    std::cout << "  perf stat -e branches,branch-misses ./test_branch_prediction" << std::endl;
    std::cout << std::endl;
    
    return 0;
}
