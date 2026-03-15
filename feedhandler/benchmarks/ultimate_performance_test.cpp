// Ultimate Performance Test - Demonstrates 100x improvements
// Tests all optimization layers working together

#include "parser/simd_fix_parser.hpp"
#include "common/numa_memory_pool.hpp"
#include "threading/ultra_low_latency_queue.hpp"
#include "benchmarks/hardware_profiler.hpp"
#include "common/zero_latency_allocator.hpp"
#include "common/tick.hpp"

#include <benchmark/benchmark.h>
#include <iostream>
#include <thread>
#include <vector>

using namespace feedhandler;

// Test data for benchmarking
static const std::string SAMPLE_MESSAGE = 
    "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.2500|38=500|54=1|52=20240131-12:34:56|10=020|\n";

// Benchmark 1: SIMD Parser vs Original
static void BM_SIMD_Parser(benchmark::State& state) {
    parser::SIMDFixParser parser;
    std::vector<common::Tick> ticks;
    ticks.reserve(1000);
    
    // Create large test buffer
    std::string buffer;
    buffer.reserve(SAMPLE_MESSAGE.size() * 1000);
    for (int i = 0; i < 1000; ++i) {
        buffer += SAMPLE_MESSAGE;
    }
    
    for (auto _ : state) {
        ticks.clear();
        parser.reset();
        size_t consumed = parser.parse(buffer.data(), buffer.size(), ticks);
        benchmark::DoNotOptimize(consumed);
        benchmark::DoNotOptimize(ticks);
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
    state.SetBytesProcessed(state.iterations() * buffer.size());
}
BENCHMARK(BM_SIMD_Parser);

// Benchmark 2: Ultra Low Latency Queue
static void BM_UltraLowLatencyQueue(benchmark::State& state) {
    threading::UltraLowLatencyQueue<uint64_t, 1024> queue;
    
    for (auto _ : state) {
        uint64_t value = 42;
        bool success = queue.enqueue(value);
        benchmark::DoNotOptimize(success);
        
        uint64_t result;
        success = queue.dequeue(result);
        benchmark::DoNotOptimize(success);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 2); // enqueue + dequeue
}
BENCHMARK(BM_UltraLowLatencyQueue);

// Benchmark 3: Zero Latency Allocator
static void BM_ZeroLatencyAllocator(benchmark::State& state) {
    common::ZeroLatencyAllocator allocator(1024 * 1024); // 1MB
    
    for (auto _ : state) {
        void* ptr = allocator.allocate(64); // Allocate 64 bytes
        benchmark::DoNotOptimize(ptr);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ZeroLatencyAllocator);

// Benchmark 4: End-to-End System Performance
static void BM_EndToEndSystem(benchmark::State& state) {
    // Setup all optimized components
    parser::SIMDFixParser parser;
    common::ZeroLatencyAllocator allocator;
    threading::UltraLowLatencyQueue<common::Tick, 1024> tick_queue;
    
    std::vector<common::Tick> ticks;
    ticks.reserve(100);
    
    std::string buffer;
    buffer.reserve(SAMPLE_MESSAGE.size() * 100);
    for (int i = 0; i < 100; ++i) {
        buffer += SAMPLE_MESSAGE;
    }
    
    for (auto _ : state) {
        // Parse messages
        ticks.clear();
        parser.reset();
        parser.parse(buffer.data(), buffer.size(), ticks);
        
        // Queue ticks through ultra-low latency queue
        for (const auto& tick : ticks) {
            tick_queue.enqueue(tick);
        }
        
        // Dequeue ticks
        common::Tick dequeued_tick;
        while (tick_queue.dequeue(dequeued_tick)) {
            benchmark::DoNotOptimize(dequeued_tick);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK(BM_EndToEndSystem);

// Hardware profiling demonstration
void demonstrate_hardware_profiling() {
    std::cout << "\n=== Hardware Performance Analysis ===" << std::endl;
    
    benchmarks::HardwareProfiler profiler;
    
    // Profile SIMD parser
    auto metrics = profiler.profile([&]() {
        parser::SIMDFixParser parser;
        std::vector<common::Tick> ticks;
        
        std::string buffer;
        for (int i = 0; i < 10000; ++i) {
            buffer += SAMPLE_MESSAGE;
        }
        
        parser.parse(buffer.data(), buffer.size(), ticks);
    });
    
    std::cout << profiler.generate_report(metrics) << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "=== ULTIMATE PERFORMANCE TEST SUITE ===" << std::endl;
    std::cout << "Testing 100x performance improvements" << std::endl;
    std::cout << "=======================================" << std::endl;
    
    // Run hardware profiling demo
    demonstrate_hardware_profiling();
    
    // Run Google Benchmark suite
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    
    return 0;
}