#pragma once

#include "common/tick.hpp"
#include <vector>
#include <cstddef>

namespace feedhandler {
namespace common {

// Object pool for zero-allocation Tick management
// Preallocates a fixed pool of Tick objects that can be reused
class TickPool {
public:
    explicit TickPool(size_t capacity = 1024);
    
    // Get next available tick slot (does not allocate)
    Tick* acquire();
    
    // Reset pool for reuse (does not deallocate)
    void reset();
    
    // Get all active ticks
    const std::vector<Tick>& get_ticks() const { return pool_; }
    
    // Get number of ticks currently in use
    size_t size() const { return next_index_; }
    
    // Get pool capacity
    size_t capacity() const { return pool_.capacity(); }
    
    // Check if pool is full
    bool is_full() const { return next_index_ >= pool_.size(); }

private:
    std::vector<Tick> pool_;      // Preallocated tick storage
    size_t next_index_;           // Next available slot
};

} // namespace common
} // namespace feedhandler
