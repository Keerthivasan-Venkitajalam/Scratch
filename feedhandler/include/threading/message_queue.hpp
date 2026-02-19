#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <cstdint>
#include <optional>

namespace feedhandler {
namespace threading {

/**
 * @brief Thread-safe message queue for passing data between network and parser threads
 * 
 * This is a simple mutex-based queue implementation. In production, consider:
 * - Lock-free queues (boost::lockfree::spsc_queue)
 * - Ring buffers with atomic operations
 * - Memory-mapped shared memory
 */
template<typename T>
class MessageQueue {
public:
    /**
     * @brief Constructor
     * @param max_size Maximum queue size (0 = unlimited)
     */
    explicit MessageQueue(size_t max_size = 0) 
        : max_size_(max_size)
        , shutdown_(false) {}
    
    /**
     * @brief Push item to queue (blocking if full)
     * @param item Item to push
     * @return true if pushed, false if queue is shutdown
     */
    bool push(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // Wait if queue is full
        if (max_size_ > 0) {
            not_full_.wait(lock, [this] { 
                return queue_.size() < max_size_ || shutdown_; 
            });
        }
        
        if (shutdown_) return false;
        
        queue_.push(item);
        not_empty_.notify_one();
        return true;
    }
    
    /**
     * @brief Push item to queue (move semantics)
     */
    bool push(T&& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (max_size_ > 0) {
            not_full_.wait(lock, [this] { 
                return queue_.size() < max_size_ || shutdown_; 
            });
        }
        
        if (shutdown_) return false;
        
        queue_.push(std::move(item));
        not_empty_.notify_one();
        return true;
    }
    
    /**
     * @brief Try to push item without blocking
     * @return true if pushed, false if queue is full or shutdown
     */
    bool try_push(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (shutdown_) return false;
        if (max_size_ > 0 && queue_.size() >= max_size_) return false;
        
        queue_.push(item);
        not_empty_.notify_one();
        return true;
    }
    
    /**
     * @brief Pop item from queue (blocking if empty)
     * @return Item if available, std::nullopt if queue is shutdown
     */
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // Wait for item or shutdown
        not_empty_.wait(lock, [this] { 
            return !queue_.empty() || shutdown_; 
        });
        
        if (queue_.empty()) return std::nullopt;
        
        T item = std::move(queue_.front());
        queue_.pop();
        
        if (max_size_ > 0) {
            not_full_.notify_one();
        }
        
        return item;
    }
    
    /**
     * @brief Try to pop item without blocking
     * @return Item if available, std::nullopt if queue is empty
     */
    std::optional<T> try_pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (queue_.empty()) return std::nullopt;
        
        T item = std::move(queue_.front());
        queue_.pop();
        
        if (max_size_ > 0) {
            not_full_.notify_one();
        }
        
        return item;
    }
    
    /**
     * @brief Get current queue size
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    /**
     * @brief Check if queue is empty
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    
    /**
     * @brief Shutdown queue (unblocks all waiting threads)
     */
    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
        not_empty_.notify_all();
        not_full_.notify_all();
    }
    
    /**
     * @brief Check if queue is shutdown
     */
    bool is_shutdown() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return shutdown_;
    }
    
    /**
     * @brief Clear all items from queue
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
        not_full_.notify_all();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::queue<T> queue_;
    size_t max_size_;
    bool shutdown_;
};

/**
 * @brief Message buffer for passing raw data between threads
 */
struct MessageBuffer {
    std::vector<char> data;
    size_t length;
    
    MessageBuffer() : length(0) {}
    
    explicit MessageBuffer(size_t capacity) : data(capacity), length(0) {}
    
    MessageBuffer(const char* buf, size_t len) : data(buf, buf + len), length(len) {}
};

} // namespace threading
} // namespace feedhandler
