// Test program for multi-threaded feed handler
// Demonstrates network thread + parser thread architecture

#include "threading/threaded_feedhandler.hpp"
#include "common/tick.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <iomanip>

using namespace feedhandler;

std::atomic<size_t> ticks_received{0};

void tick_callback(const common::Tick& tick) {
    ticks_received.fetch_add(1);
    
    // Print first few ticks
    if (ticks_received.load() <= 5) {
        std::cout << "[Callback] Tick #" << ticks_received.load() << ": "
                  << std::string(tick.symbol.data(), tick.symbol.size())
                  << " @ " << common::price_to_double(tick.price)
                  << " x " << tick.qty
                  << " (" << tick.side << ")"
                  << std::endl;
    }
}

void print_separator() {
    std::cout << "========================================" << std::endl;
}

void test_basic_threading() {
    print_separator();
    std::cout << "Test 1: Basic Threading" << std::endl;
    print_separator();
    
    threading::ThreadedFeedHandler::Config config;
    config.queue_size = 100;
    config.buffer_size = 8192;
    
    threading::ThreadedFeedHandler handler(config, tick_callback);
    
    std::cout << "Starting handler..." << std::endl;
    handler.start();
    
    // Give threads time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Inject some test messages
    std::string msg1 = "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.25|38=500|54=1|52=20240131-12:34:56|10=020|\n";
    std::string msg2 = "8=FIX.4.4|9=79|35=D|55=GOOGL|44=2800.50|38=100|54=2|52=20240131-12:34:57|10=021|\n";
    std::string msg3 = "8=FIX.4.4|9=79|35=D|55=TSLA|44=245.75|38=750|54=1|52=20240131-12:34:58|10=022|\n";
    
    handler.inject_data(msg1.data(), msg1.size());
    handler.inject_data(msg2.data(), msg2.size());
    handler.inject_data(msg3.data(), msg3.size());
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    std::cout << "\nStopping handler..." << std::endl;
    handler.stop();
    
    const auto& stats = handler.get_statistics();
    std::cout << "\nStatistics:" << std::endl;
    std::cout << "  Bytes received: " << stats.bytes_received.load() << std::endl;
    std::cout << "  Messages parsed: " << stats.messages_parsed.load() << std::endl;
    std::cout << "  Parse errors: " << stats.parse_errors.load() << std::endl;
    std::cout << "  Queue overflows: " << stats.queue_overflows.load() << std::endl;
    std::cout << std::endl;
}

void test_high_throughput() {
    print_separator();
    std::cout << "Test 2: High Throughput" << std::endl;
    print_separator();
    
    ticks_received.store(0);
    
    threading::ThreadedFeedHandler::Config config;
    config.queue_size = 1000;
    
    threading::ThreadedFeedHandler handler(config, tick_callback);
    handler.start();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Inject many messages rapidly
    std::string msg = "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.25|38=500|54=1|52=20240131-12:34:56|10=020|\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    const size_t message_count = 10000;
    for (size_t i = 0; i < message_count; ++i) {
        handler.inject_data(msg.data(), msg.size());
    }
    
    // Wait for all messages to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    handler.stop();
    
    const auto& stats = handler.get_statistics();
    
    std::cout << "\nHigh Throughput Results:" << std::endl;
    std::cout << "  Messages injected: " << message_count << std::endl;
    std::cout << "  Messages parsed: " << stats.messages_parsed.load() << std::endl;
    std::cout << "  Time: " << duration.count() << " ms" << std::endl;
    std::cout << "  Throughput: " << (stats.messages_parsed.load() * 1000 / duration.count()) 
              << " msg/sec" << std::endl;
    std::cout << "  Queue overflows: " << stats.queue_overflows.load() << std::endl;
    std::cout << std::endl;
}

void test_queue_backpressure() {
    print_separator();
    std::cout << "Test 3: Queue Backpressure" << std::endl;
    print_separator();
    
    ticks_received.store(0);
    
    threading::ThreadedFeedHandler::Config config;
    config.queue_size = 10;  // Small queue to trigger backpressure
    
    threading::ThreadedFeedHandler handler(config, tick_callback);
    handler.start();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Inject messages faster than parser can consume
    std::string msg = "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.25|38=500|54=1|52=20240131-12:34:56|10=020|\n";
    
    const size_t message_count = 100;
    for (size_t i = 0; i < message_count; ++i) {
        handler.inject_data(msg.data(), msg.size());
    }
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    handler.stop();
    
    const auto& stats = handler.get_statistics();
    
    std::cout << "\nBackpressure Results:" << std::endl;
    std::cout << "  Messages injected: " << message_count << std::endl;
    std::cout << "  Messages parsed: " << stats.messages_parsed.load() << std::endl;
    std::cout << "  Queue overflows: " << stats.queue_overflows.load() 
              << " (expected with small queue)" << std::endl;
    std::cout << std::endl;
}

void test_garbage_recovery_threaded() {
    print_separator();
    std::cout << "Test 4: Garbage Recovery (Threaded)" << std::endl;
    print_separator();
    
    ticks_received.store(0);
    
    threading::ThreadedFeedHandler::Config config;
    config.enable_garbage_recovery = true;
    
    threading::ThreadedFeedHandler handler(config, tick_callback);
    handler.start();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Inject garbage followed by valid message
    std::string garbage = "CORRUPT_DATA_HERE!!!";
    std::string valid = "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.25|38=500|54=1|52=20240131-12:34:56|10=020|\n";
    
    handler.inject_data(garbage.data(), garbage.size());
    handler.inject_data(valid.data(), valid.size());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    handler.stop();
    
    const auto& stats = handler.get_statistics();
    
    std::cout << "\nGarbage Recovery Results:" << std::endl;
    std::cout << "  Messages parsed: " << stats.messages_parsed.load() 
              << " (should be 1 despite garbage)" << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "\n";
    print_separator();
    std::cout << "Threaded FeedHandler Tests" << std::endl;
    print_separator();
    std::cout << "\n";
    
    test_basic_threading();
    test_high_throughput();
    test_queue_backpressure();
    test_garbage_recovery_threaded();
    
    print_separator();
    std::cout << "All tests complete!" << std::endl;
    print_separator();
    std::cout << "\n";
    
    return 0;
}
