#include "benchmarks/hardware_profiler.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace feedhandler::benchmarks;

void cpu_intensive_task() {
    // CPU-bound task with good branch prediction
    volatile double result = 0.0;
    for (int i = 0; i < 1000000; ++i) {
        result += std::sqrt(i * 3.14159);
    }
}

void cache_unfriendly_task() {
    // Task that causes cache misses
    const size_t array_size = 1024 * 1024; // 1MB array
    std::vector<int> large_array(array_size);
    
    // Random access pattern to cause cache misses
    volatile int sum = 0;
    for (int i = 0; i < 100000; ++i) {
        size_t index = (i * 7919) % array_size; // Prime number for pseudo-random access
        sum += large_array[index];
    }
}

void branch_unpredictable_task() {
    // Task with unpredictable branches
    std::vector<int> data(100000);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = rand() % 1000;
    }
    
    volatile int sum = 0;
    for (int value : data) {
        if (value > 500) {  // Unpredictable branch
            sum += value * 2;
        } else {
            sum += value / 2;
        }
    }
}

void test_basic_profiling() {
    std::cout << "Testing basic hardware profiling..." << std::endl;
    
    HardwareProfiler profiler;
    
    auto metrics = profiler.profile([]() {
        cpu_intensive_task();
    });
    
    std::cout << "\nCPU Intensive Task Results:" << std::endl;
    std::cout << profiler.generate_report(metrics) << std::endl;
}

void test_cache_performance() {
    std::cout << "\nTesting cache performance profiling..." << std::endl;
    
    HardwareProfiler profiler;
    
    auto metrics = profiler.profile([]() {
        cache_unfriendly_task();
    });
    
    std::cout << "\nCache Unfriendly Task Results:" << std::endl;
    std::cout << profiler.generate_report(metrics) << std::endl;
}

void test_branch_prediction() {
    std::cout << "\nTesting branch prediction profiling..." << std::endl;
    
    HardwareProfiler profiler;
    
    auto metrics = profiler.profile([]() {
        branch_unpredictable_task();
    });
    
    std::cout << "\nBranch Unpredictable Task Results:" << std::endl;
    std::cout << profiler.generate_report(metrics) << std::endl;
}

void test_manual_profiling() {
    std::cout << "\nTesting manual start/stop profiling..." << std::endl;
    
    HardwareProfiler profiler;
    
    profiler.start();
    
    // Do some work
    volatile int sum = 0;
    for (int i = 0; i < 1000000; ++i) {
        sum += i;
    }
    
    auto metrics = profiler.stop();
    
    std::cout << "\nManual Profiling Results:" << std::endl;
    std::cout << profiler.generate_report(metrics) << std::endl;
}

void compare_algorithms() {
    std::cout << "\nComparing algorithm performance..." << std::endl;
    
    HardwareProfiler profiler;
    
    // Algorithm 1: Simple loop
    auto metrics1 = profiler.profile([]() {
        volatile int sum = 0;
        for (int i = 0; i < 1000000; ++i) {
            sum += i;
        }
    });
    
    // Algorithm 2: Unrolled loop
    auto metrics2 = profiler.profile([]() {
        volatile int sum = 0;
        for (int i = 0; i < 1000000; i += 4) {
            sum += i;
            sum += i + 1;
            sum += i + 2;
            sum += i + 3;
        }
    });
    
    std::cout << "\nSimple Loop:" << std::endl;
    std::cout << profiler.generate_report(metrics1) << std::endl;
    
    std::cout << "\nUnrolled Loop:" << std::endl;
    std::cout << profiler.generate_report(metrics2) << std::endl;
    
    // Compare performance
    double speedup = static_cast<double>(metrics1.wall_time.count()) / metrics2.wall_time.count();
    std::cout << "\nSpeedup from loop unrolling: " << speedup << "x" << std::endl;
}

int main() {
    std::cout << "=== Hardware Profiler Test Suite ===" << std::endl;
    
    try {
        test_basic_profiling();
        test_cache_performance();
        test_branch_prediction();
        test_manual_profiling();
        compare_algorithms();
        
        std::cout << "\n✅ All hardware profiler tests completed!" << std::endl;
        std::cout << "\nNote: Some metrics may be zero if perf counters are not available" << std::endl;
        std::cout << "Run with sudo or enable perf counters for full functionality" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}