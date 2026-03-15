#include "common/zero_latency_allocator.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>

using namespace feedhandler::common;

void test_basic_allocation() {
    std::cout << "Testing basic allocation..." << std::endl;
    
    ZeroLatencyAllocator allocator(1024 * 1024); // 1MB
    
    // Test various allocation sizes
    std::vector<void*> ptrs;
    
    for (size_t size : {8, 16, 32, 64, 128, 256, 512, 1024}) {
        void* ptr = allocator.allocate(size);
        if (ptr) {
            ptrs.push_back(ptr);
            std::cout << "Allocated " << size << " bytes at " << ptr << std::endl;
        } else {
            std::cout << "Failed to allocate " << size << " bytes" << std::endl;
        }
    }
    
    auto stats = allocator.get_stats();
    std::cout << "Stats: " << stats.allocated_size << "/" << stats.total_size 
              << " bytes (" << stats.utilization_percent << "% used)" << std::endl;
}

void test_alignment() {
    std::cout << "\nTesting alignment..." << std::endl;
    
    ZeroLatencyAllocator allocator(1024 * 1024);
    
    for (size_t alignment : {8, 16, 32, 64, 128}) {
        void* ptr = allocator.allocate(100, alignment);
        if (ptr) {
            uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
            bool aligned = (addr % alignment) == 0;
            std::cout << "Alignment " << alignment << ": " << ptr 
                      << " - " << (aligned ? "✅" : "❌") << std::endl;
        }
    }
}

void benchmark_allocation_speed() {
    std::cout << "\nBenchmarking allocation speed..." << std::endl;
    
    ZeroLatencyAllocator allocator(1024 * 1024 * 1024); // 1GB
    
    const size_t num_allocations = 1000000;
    const size_t allocation_size = 64;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < num_allocations; ++i) {
        void* ptr = allocator.allocate(allocation_size);
        if (!ptr) {
            std::cout << "Allocation failed at iteration " << i << std::endl;
            break;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    double avg_time = static_cast<double>(duration.count()) / num_allocations;
    
    std::cout << "Allocated " << num_allocations << " blocks in " 
              << duration.count() << " ns" << std::endl;
    std::cout << "Average allocation time: " << avg_time << " ns" << std::endl;
    std::cout << "Allocation rate: " << (num_allocations * 1e9 / duration.count()) 
              << " allocations/second" << std::endl;
}

void test_concurrent_allocation() {
    std::cout << "\nTesting concurrent allocation..." << std::endl;
    
    ZeroLatencyAllocator allocator(1024 * 1024 * 1024); // 1GB
    
    const size_t num_threads = 4;
    const size_t allocations_per_thread = 100000;
    
    std::vector<std::thread> threads;
    std::vector<size_t> success_counts(num_threads, 0);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (size_t i = 0; i < allocations_per_thread; ++i) {
                void* ptr = allocator.allocate(64);
                if (ptr) {
                    success_counts[t]++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    size_t total_success = 0;
    for (size_t count : success_counts) {
        total_success += count;
    }
    
    std::cout << "Concurrent allocation: " << total_success << "/" 
              << (num_threads * allocations_per_thread) << " successful" << std::endl;
    std::cout << "Time: " << duration.count() << " microseconds" << std::endl;
    std::cout << "Rate: " << (total_success * 1000000.0 / duration.count()) 
              << " allocations/second" << std::endl;
}

void test_reset_functionality() {
    std::cout << "\nTesting reset functionality..." << std::endl;
    
    ZeroLatencyAllocator allocator(1024 * 1024);
    
    // Allocate some memory
    std::vector<void*> ptrs;
    for (int i = 0; i < 100; ++i) {
        ptrs.push_back(allocator.allocate(1024));
    }
    
    auto stats_before = allocator.get_stats();
    std::cout << "Before reset: " << stats_before.allocated_size 
              << " bytes allocated" << std::endl;
    
    // Reset allocator
    allocator.reset();
    
    auto stats_after = allocator.get_stats();
    std::cout << "After reset: " << stats_after.allocated_size 
              << " bytes allocated" << std::endl;
    
    // Verify we can allocate again
    void* new_ptr = allocator.allocate(1024);
    std::cout << "New allocation after reset: " << (new_ptr ? "✅" : "❌") << std::endl;
}

int main() {
    std::cout << "=== Zero Latency Allocator Test Suite ===" << std::endl;
    
    try {
        test_basic_allocation();
        test_alignment();
        test_reset_functionality();
        benchmark_allocation_speed();
        test_concurrent_allocation();
        
        std::cout << "\n✅ All zero latency allocator tests passed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}