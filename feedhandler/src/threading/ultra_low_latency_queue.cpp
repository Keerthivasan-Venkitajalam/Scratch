#include "threading/ultra_low_latency_queue.hpp"
#include <new>

namespace feedhandler {
namespace threading {

template<typename T, size_t Size>
UltraLowLatencyQueue<T, Size>::UltraLowLatencyQueue() 
    : head_(0), tail_(0) {
    // Initialize data array
    for (size_t i = 0; i < Size; ++i) {
        new (&data_[i]) T();
    }
}

template<typename T, size_t Size>
bool UltraLowLatencyQueue<T, Size>::enqueue(const T& item) noexcept {
    const size_t current_tail = tail_.load(std::memory_order_relaxed);
    const size_t next_tail = (current_tail + 1) & MASK;
    
    // Check if queue is full
    if (next_tail == head_.load(std::memory_order_acquire)) {
        return false;
    }
    
    // Prefetch next cache line for better performance
    prefetch_for_write(&data_[(next_tail + 1) & MASK]);
    
    // Store item
    data_[current_tail] = item;
    
    // Update tail with release semantics
    tail_.store(next_tail, std::memory_order_release);
    
    return true;
}

template<typename T, size_t Size>
bool UltraLowLatencyQueue<T, Size>::dequeue(T& item) noexcept {
    const size_t current_head = head_.load(std::memory_order_relaxed);
    
    // Check if queue is empty
    if (current_head == tail_.load(std::memory_order_acquire)) {
        return false;
    }
    
    // Prefetch next cache line
    prefetch_for_read(&data_[(current_head + 1) & MASK]);
    
    // Load item
    item = data_[current_head];
    
    // Update head with release semantics
    head_.store((current_head + 1) & MASK, std::memory_order_release);
    
    return true;
}

template<typename T, size_t Size>
bool UltraLowLatencyQueue<T, Size>::empty() const noexcept {
    return head_.load(std::memory_order_relaxed) == 
           tail_.load(std::memory_order_relaxed);
}

template<typename T, size_t Size>
size_t UltraLowLatencyQueue<T, Size>::size() const noexcept {
    const size_t current_tail = tail_.load(std::memory_order_relaxed);
    const size_t current_head = head_.load(std::memory_order_relaxed);
    
    return (current_tail - current_head) & MASK;
}

// Explicit template instantiations for common types
template class UltraLowLatencyQueue<int, 1024>;
template class UltraLowLatencyQueue<double, 1024>;
template class UltraLowLatencyQueue<uint64_t, 1024>;

} // namespace threading
} // namespace feedhandler