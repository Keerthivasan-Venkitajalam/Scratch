// Test program for Object Pool and Flyweight patterns
// Demonstrates zero-allocation tick management

#include "common/tick_pool.hpp"
#include "common/flyweight_tick.hpp"
#include "common/tick.hpp"

#include <iostream>
#include <chrono>
#include <cstring>

using namespace feedhandler::common;

void test_object_pool() {
    std::cout << "========================================" << std::endl;
    std::cout << "Object Pool Pattern Test" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Create pool with capacity for 1000 ticks
    TickPool pool(1000);
    
    std::cout << "Pool capacity: " << pool.capacity() << std::endl;
    std::cout << "Pool size: " << pool.size() << std::endl;
    std::cout << std::endl;
    
    // Simulate parsing messages and acquiring ticks
    std::cout << "Acquiring 5 ticks from pool..." << std::endl;
    
    for (int i = 0; i < 5; ++i) {
        Tick* tick = pool.acquire();
        if (tick) {
            // Simulate parser filling in tick data
            char symbol[8];
            snprintf(symbol, sizeof(symbol), "SYM%d", i);
            tick->copy_symbol(symbol);
            tick->price = 100000 + i * 1000;  // Fixed-point: 10.00, 10.10, etc.
            tick->qty = 100 * (i + 1);
            tick->side = (i % 2 == 0) ? 'B' : 'S';
            tick->timestamp = Tick::current_timestamp_ns();
            
            std::cout << "  Tick " << i << ": " 
                      << tick->symbol << " "
                      << price_to_double(tick->price) << " "
                      << tick->qty << " "
                      << tick->side << std::endl;
        }
    }
    
    std::cout << "Pool size after acquiring: " << pool.size() << std::endl;
    std::cout << std::endl;
    
    // Reset pool for reuse
    std::cout << "Resetting pool..." << std::endl;
    pool.reset();
    std::cout << "Pool size after reset: " << pool.size() << std::endl;
    std::cout << std::endl;
}

void test_flyweight_pattern() {
    std::cout << "========================================" << std::endl;
    std::cout << "Flyweight Pattern Test" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Simulate receive buffer (this would come from network)
    const char* buffer = "AAPL|150.25|500|B|MSFT|280.50|1000|S|GOOGL|2800.75|250|B";
    size_t buffer_len = strlen(buffer);
    
    std::cout << "Buffer: " << buffer << std::endl;
    std::cout << "Buffer size: " << buffer_len << " bytes" << std::endl;
    std::cout << std::endl;
    
    // Create flyweight pool
    FlyweightTickPool pool(100);
    
    std::cout << "Flyweight tick size: " << sizeof(FlyweightTick) << " bytes" << std::endl;
    std::cout << "Regular tick size: " << sizeof(Tick) << " bytes" << std::endl;
    std::cout << "Memory savings: " << (sizeof(Tick) - sizeof(FlyweightTick)) << " bytes per tick" << std::endl;
    std::cout << std::endl;
    
    // Parse buffer and create flyweight ticks
    std::cout << "Creating flyweight ticks (pointing into buffer)..." << std::endl;
    
    const char* ptr = buffer;
    int tick_count = 0;
    
    while (ptr < buffer + buffer_len) {
        FlyweightTick* tick = pool.acquire();
        if (!tick) break;
        
        // Find symbol (up to '|')
        const char* symbol_start = ptr;
        const char* symbol_end = strchr(ptr, '|');
        if (!symbol_end) break;
        
        tick->symbol = std::string_view(symbol_start, symbol_end - symbol_start);
        ptr = symbol_end + 1;
        
        // Parse price (simplified - just for demo)
        double price = atof(ptr);
        tick->price = double_to_price(price);
        ptr = strchr(ptr, '|') + 1;
        
        // Parse qty
        tick->qty = atoi(ptr);
        ptr = strchr(ptr, '|') + 1;
        
        // Parse side
        tick->side = *ptr;
        ptr += 2;  // Skip side and delimiter
        
        tick->timestamp = Tick::current_timestamp_ns();
        
        std::cout << "  Tick " << tick_count++ << ": "
                  << tick->symbol << " "
                  << price_to_double(tick->price) << " "
                  << tick->qty << " "
                  << tick->side << std::endl;
    }
    
    std::cout << "Total ticks created: " << pool.size() << std::endl;
    std::cout << std::endl;
    
    // IMPORTANT: All ticks become invalid when buffer is recycled!
    std::cout << "WARNING: Flyweight ticks are only valid while buffer exists!" << std::endl;
    std::cout << std::endl;
}

void benchmark_allocation() {
    std::cout << "========================================" << std::endl;
    std::cout << "Allocation Benchmark" << std::endl;
    std::cout << "========================================" << std::endl;
    
    const size_t iterations = 1000000;
    
    // Benchmark 1: Traditional allocation (vector push_back)
    {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<Tick> ticks;
        for (size_t i = 0; i < iterations; ++i) {
            Tick tick;
            tick.copy_symbol("AAPL");
            tick.price = 1500000;
            tick.qty = 100;
            tick.side = 'B';
            tick.timestamp = Tick::current_timestamp_ns();
            ticks.push_back(tick);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Traditional allocation (push_back):" << std::endl;
        std::cout << "  Time: " << duration.count() << " μs" << std::endl;
        std::cout << "  Ticks: " << ticks.size() << std::endl;
    }
    
    // Benchmark 2: Object pool (preallocated)
    {
        TickPool pool(iterations);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < iterations; ++i) {
            Tick* tick = pool.acquire();
            tick->copy_symbol("AAPL");
            tick->price = 1500000;
            tick->qty = 100;
            tick->side = 'B';
            tick->timestamp = Tick::current_timestamp_ns();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Object pool (preallocated):" << std::endl;
        std::cout << "  Time: " << duration.count() << " μs" << std::endl;
        std::cout << "  Ticks: " << pool.size() << std::endl;
    }
    
    // Benchmark 3: Flyweight pool
    {
        FlyweightTickPool pool(iterations);
        const char* symbol_buffer = "AAPL";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < iterations; ++i) {
            FlyweightTick* tick = pool.acquire();
            tick->symbol = std::string_view(symbol_buffer, 4);
            tick->price = 1500000;
            tick->qty = 100;
            tick->side = 'B';
            tick->timestamp = Tick::current_timestamp_ns();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Flyweight pool (zero-copy):" << std::endl;
        std::cout << "  Time: " << duration.count() << " μs" << std::endl;
        std::cout << "  Ticks: " << pool.size() << std::endl;
    }
    
    std::cout << std::endl;
}

int main() {
    test_object_pool();
    test_flyweight_pattern();
    benchmark_allocation();
    
    std::cout << "========================================" << std::endl;
    std::cout << "Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Object Pool: Preallocated storage, no runtime allocation" << std::endl;
    std::cout << "Flyweight: Zero-copy, minimal memory, buffer-lifetime dependent" << std::endl;
    std::cout << std::endl;
    std::cout << "Use Object Pool when: Ticks need to outlive buffer" << std::endl;
    std::cout << "Use Flyweight when: Maximum performance, buffer lifetime managed" << std::endl;
    std::cout << std::endl;
    
    return 0;
}
