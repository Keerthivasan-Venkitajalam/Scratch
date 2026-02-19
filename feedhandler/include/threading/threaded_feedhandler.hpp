#pragma once

#include "threading/message_queue.hpp"
#include "parser/fsm_fix_parser.hpp"
#include "common/tick.hpp"

#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <memory>

namespace feedhandler {
namespace threading {

/**
 * @brief Multi-threaded feed handler with separate network and parser threads
 * 
 * Architecture:
 * - Network Thread: Reads from socket, pushes raw buffers to queue
 * - Parser Thread: Pops buffers from queue, parses into Ticks
 * - Main Thread: Consumes parsed Ticks
 * 
 * This is a simple mutex-based implementation. For production:
 * - Use lock-free queues (SPSC ring buffer)
 * - Consider NUMA-aware thread pinning
 * - Implement backpressure handling
 */
class ThreadedFeedHandler {
public:
    /**
     * @brief Statistics for monitoring
     */
    struct Statistics {
        std::atomic<uint64_t> bytes_received{0};
        std::atomic<uint64_t> messages_parsed{0};
        std::atomic<uint64_t> parse_errors{0};
        std::atomic<uint64_t> queue_overflows{0};
        std::atomic<uint64_t> network_reads{0};
        std::atomic<uint64_t> parser_cycles{0};
        
        // Delete copy constructor and assignment (atomics are not copyable)
        Statistics() = default;
        Statistics(const Statistics&) = delete;
        Statistics& operator=(const Statistics&) = delete;
    };
    
    /**
     * @brief Configuration
     */
    struct Config {
        size_t queue_size = 1000;           // Max buffers in queue
        size_t buffer_size = 8192;          // Size of each buffer
        bool enable_garbage_recovery = true; // Enable parser recovery
        
        Config() = default;
    };
    
    /**
     * @brief Callback for parsed ticks
     */
    using TickCallback = std::function<void(const common::Tick&)>;
    
    /**
     * @brief Constructor
     * @param config Configuration
     * @param callback Callback for parsed ticks
     */
    ThreadedFeedHandler(const Config& config, TickCallback callback);
    
    /**
     * @brief Destructor (stops threads)
     */
    ~ThreadedFeedHandler();
    
    /**
     * @brief Start network and parser threads
     */
    void start();
    
    /**
     * @brief Stop all threads gracefully
     */
    void stop();
    
    /**
     * @brief Check if handler is running
     */
    bool is_running() const { return running_.load(); }
    
    /**
     * @brief Simulate network data (for testing)
     * @param data Raw data to inject
     * @param length Length of data
     */
    void inject_data(const char* data, size_t length);
    
    /**
     * @brief Get statistics
     */
    const Statistics& get_statistics() const { return stats_; }
    
    /**
     * @brief Reset statistics
     */
    void reset_statistics();

private:
    /**
     * @brief Network thread function (simulated for now)
     */
    void network_thread_func();
    
    /**
     * @brief Parser thread function
     */
    void parser_thread_func();
    
    // Configuration
    Config config_;
    TickCallback tick_callback_;
    
    // Threading
    std::unique_ptr<std::thread> network_thread_;
    std::unique_ptr<std::thread> parser_thread_;
    std::atomic<bool> running_;
    
    // Queue for passing buffers between threads
    MessageQueue<MessageBuffer> buffer_queue_;
    
    // Statistics
    Statistics stats_;
    
    // Parser (owned by parser thread)
    parser::FSMFixParser parser_;
};

} // namespace threading
} // namespace feedhandler
