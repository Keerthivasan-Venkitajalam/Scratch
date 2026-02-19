#include "threading/threaded_feedhandler.hpp"

#include <iostream>
#include <chrono>
#include <thread>

namespace feedhandler {
namespace threading {

ThreadedFeedHandler::ThreadedFeedHandler(const Config& config, TickCallback callback)
    : config_(config)
    , tick_callback_(std::move(callback))
    , running_(false)
    , buffer_queue_(config.queue_size) {
    
    parser_.set_garbage_recovery(config.enable_garbage_recovery);
}

ThreadedFeedHandler::~ThreadedFeedHandler() {
    stop();
}

void ThreadedFeedHandler::start() {
    if (running_.load()) {
        return;  // Already running
    }
    
    running_.store(true);
    
    // Start parser thread first
    parser_thread_ = std::make_unique<std::thread>(&ThreadedFeedHandler::parser_thread_func, this);
    
    // Start network thread
    network_thread_ = std::make_unique<std::thread>(&ThreadedFeedHandler::network_thread_func, this);
    
    std::cout << "[ThreadedFeedHandler] Started network and parser threads" << std::endl;
}

void ThreadedFeedHandler::stop() {
    if (!running_.load()) {
        return;  // Not running
    }
    
    std::cout << "[ThreadedFeedHandler] Stopping threads..." << std::endl;
    
    running_.store(false);
    buffer_queue_.shutdown();
    
    // Wait for threads to finish
    if (network_thread_ && network_thread_->joinable()) {
        network_thread_->join();
    }
    
    if (parser_thread_ && parser_thread_->joinable()) {
        parser_thread_->join();
    }
    
    std::cout << "[ThreadedFeedHandler] Threads stopped" << std::endl;
}

void ThreadedFeedHandler::inject_data(const char* data, size_t length) {
    if (!running_.load()) {
        return;
    }
    
    // Create buffer and push to queue
    MessageBuffer buffer(data, length);
    
    if (!buffer_queue_.try_push(std::move(buffer))) {
        stats_.queue_overflows.fetch_add(1);
    }
    
    stats_.bytes_received.fetch_add(length);
}

void ThreadedFeedHandler::reset_statistics() {
    stats_.bytes_received.store(0);
    stats_.messages_parsed.store(0);
    stats_.parse_errors.store(0);
    stats_.queue_overflows.store(0);
    stats_.network_reads.store(0);
    stats_.parser_cycles.store(0);
}

void ThreadedFeedHandler::network_thread_func() {
    std::cout << "[NetworkThread] Started" << std::endl;
    
    // In a real implementation, this would:
    // 1. Read from socket using recv()
    // 2. Handle non-blocking I/O with select()/epoll()
    // 3. Manage receive buffer
    // 4. Push complete or partial messages to queue
    
    // For now, this thread just waits for injected data
    // The inject_data() method simulates network reads
    
    while (running_.load()) {
        stats_.network_reads.fetch_add(1);
        
        // Simulate network polling delay
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::cout << "[NetworkThread] Stopped" << std::endl;
}

void ThreadedFeedHandler::parser_thread_func() {
    std::cout << "[ParserThread] Started" << std::endl;
    
    std::vector<common::Tick> ticks;
    ticks.reserve(100);  // Preallocate for batch processing
    
    while (running_.load() || !buffer_queue_.empty()) {
        stats_.parser_cycles.fetch_add(1);
        
        // Pop buffer from queue (blocking)
        auto buffer_opt = buffer_queue_.pop();
        
        if (!buffer_opt.has_value()) {
            // Queue shutdown
            break;
        }
        
        MessageBuffer& buffer = buffer_opt.value();
        
        // Parse buffer
        ticks.clear();
        size_t consumed = parser_.parse(buffer.data.data(), buffer.length, ticks);
        
        // Invoke callback for each parsed tick
        for (const auto& tick : ticks) {
            if (tick_callback_) {
                tick_callback_(tick);
            }
            stats_.messages_parsed.fetch_add(1);
        }
        
        // Check for parse errors (if consumed < length, might be incomplete message)
        if (consumed < buffer.length && ticks.empty()) {
            stats_.parse_errors.fetch_add(1);
        }
    }
    
    std::cout << "[ParserThread] Stopped" << std::endl;
}

} // namespace threading
} // namespace feedhandler
