#include "common/numa_memory_pool.hpp"
#include <numa.h>
#include <numaif.h>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>

namespace feedhandler {
namespace common {

template<typename T, size_t PoolSize>
NUMAMemoryPool<T, PoolSize>::NUMAMemoryPool() 
    : free_head_(nullptr), allocated_count_(0), numa_memory_(nullptr), numa_node_(0) {
    initialize_numa_pool();
}

template<typename T, size_t PoolSize>
NUMAMemoryPool<T, PoolSize>::~NUMAMemoryPool() {
    if (numa_memory_) {
        numa_free(numa_memory_, sizeof(Node) * PoolSize);
    }
}

template<typename T, size_t PoolSize>
void NUMAMemoryPool<T, PoolSize>::initialize_numa_pool() {
    // Detect current NUMA node
    numa_node_ = detect_numa_node();
    
    // Allocate memory on specific NUMA node
    numa_memory_ = numa_alloc_onnode(sizeof(Node) * PoolSize, numa_node_);
    if (!numa_memory_) {
        throw std::bad_alloc();
    }
    
    // Initialize pool as array of nodes
    pool_memory_ = static_cast<Node*>(numa_memory_);
    
    // Build free list (lock-free stack)
    for (size_t i = 0; i < PoolSize - 1; ++i) {
        pool_memory_[i].next.store(&pool_memory_[i + 1], std::memory_order_relaxed);
    }
    pool_memory_[PoolSize - 1].next.store(nullptr, std::memory_order_relaxed);
    
    // Set head of free list
    free_head_.store(&pool_memory_[0], std::memory_order_relaxed);
}

template<typename T, size_t PoolSize>
int NUMAMemoryPool<T, PoolSize>::detect_numa_node() {
    if (numa_available() < 0) {
        return 0; // NUMA not available, use node 0
    }
    
    // Get current thread's NUMA node
    int cpu = sched_getcpu();
    if (cpu >= 0) {
        return numa_node_of_cpu(cpu);
    }
    
    return 0; // Default to node 0
}

template<typename T, size_t PoolSize>
T* NUMAMemoryPool<T, PoolSize>::allocate() {
    Node* head = free_head_.load(std::memory_order_acquire);
    
    while (head != nullptr) {
        Node* next = head->next.load(std::memory_order_relaxed);
        
        // Try to update head atomically
        if (free_head_.compare_exchange_weak(head, next, 
                                           std::memory_order_release, 
                                           std::memory_order_acquire)) {
            allocated_count_.fetch_add(1, std::memory_order_relaxed);
            return &head->data;
        }
        // head was updated by compare_exchange_weak, retry
    }
    
    return nullptr; // Pool exhausted
}

template<typename T, size_t PoolSize>
void NUMAMemoryPool<T, PoolSize>::deallocate(T* ptr) {
    if (!ptr) return;
    
    // Calculate node from data pointer
    Node* node = reinterpret_cast<Node*>(
        reinterpret_cast<char*>(ptr) - offsetof(Node, data));
    
    // Verify pointer is within our pool
    if (node < pool_memory_ || node >= pool_memory_ + PoolSize) {
        return; // Invalid pointer
    }
    
    // Add back to free list (lock-free)
    Node* head = free_head_.load(std::memory_order_relaxed);
    do {
        node->next.store(head, std::memory_order_relaxed);
    } while (!free_head_.compare_exchange_weak(head, node,
                                             std::memory_order_release,
                                             std::memory_order_relaxed));
    
    allocated_count_.fetch_sub(1, std::memory_order_relaxed);
}

template<typename T, size_t PoolSize>
typename NUMAMemoryPool<T, PoolSize>::Stats 
NUMAMemoryPool<T, PoolSize>::get_stats() const {
    size_t allocated = allocated_count_.load(std::memory_order_relaxed);
    return Stats{
        .allocated_count = allocated,
        .free_count = PoolSize - allocated,
        .numa_node = static_cast<size_t>(numa_node_),
        .hit_rate = static_cast<double>(allocated) / PoolSize
    };
}

// Explicit template instantiations for common types
template class NUMAMemoryPool<int, 1024>;
template class NUMAMemoryPool<double, 1024>;
template class NUMAMemoryPool<char[64], 1024>;

} // namespace common
} // namespace feedhandler