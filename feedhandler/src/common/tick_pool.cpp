#include "common/tick_pool.hpp"

namespace feedhandler {
namespace common {

TickPool::TickPool(size_t capacity) 
    : next_index_(0) {
    // Preallocate the entire pool upfront - no allocations during parsing
    pool_.reserve(capacity);
    pool_.resize(capacity);
}

Tick* TickPool::acquire() {
    if (is_full()) {
        return nullptr;  // Pool exhausted
    }
    
    // Return pointer to next available slot
    return &pool_[next_index_++];
}

void TickPool::reset() {
    // Reset index without deallocating memory
    next_index_ = 0;
    
    // Optionally clear tick data (not strictly necessary)
    // for (auto& tick : pool_) {
    //     tick = Tick{};
    // }
}

} // namespace common
} // namespace feedhandler
