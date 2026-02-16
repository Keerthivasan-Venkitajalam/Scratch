#pragma once

#include <string_view>
#include <cstdint>

namespace feedhandler {
namespace common {

/**
 * @brief Flyweight Tick - stores only views into external buffer
 * 
 * This is a pure flyweight implementation that stores NO owned data.
 * All string_view fields point directly into the receive buffer.
 * 
 * CRITICAL: The source buffer MUST remain valid for the lifetime
 * of this tick. Do not use after buffer is recycled.
 * 
 * Memory footprint: 40 bytes (vs 88 bytes for full Tick with storage)
 */
struct FlyweightTick {
    std::string_view symbol;  ///< Points into receive buffer
    int64_t price;           ///< Fixed-point price (scaled by 10000)
    int32_t qty;             ///< Quantity
    char side;               ///< 'B' or 'S'
    uint64_t timestamp;      ///< Nanoseconds since epoch
    
    // No symbol_storage_ - pure flyweight!
    
    FlyweightTick() 
        : symbol{}, price{0}, qty{0}, side{'\0'}, timestamp{0} {}
    
    FlyweightTick(std::string_view sym, int64_t p, int32_t q, char s, uint64_t ts)
        : symbol{sym}, price{p}, qty{q}, side{s}, timestamp{ts} {}
    
    bool is_valid() const {
        return !symbol.empty() && price > 0 && qty > 0 && 
               (side == 'B' || side == 'S');
    }
    
    // Explicitly deleted copy/move to prevent dangling references
    FlyweightTick(const FlyweightTick&) = default;
    FlyweightTick(FlyweightTick&&) = default;
    FlyweightTick& operator=(const FlyweightTick&) = default;
    FlyweightTick& operator=(FlyweightTick&&) = default;
};

/**
 * @brief Flyweight Tick Pool - manages ticks with buffer lifetime
 * 
 * This pool is tied to a specific buffer lifetime. When the buffer
 * is recycled, all ticks in this pool become invalid.
 */
class FlyweightTickPool {
public:
    explicit FlyweightTickPool(size_t capacity = 1024) 
        : pool_(capacity), next_index_(0) {}
    
    // Acquire next tick slot (returns nullptr if full)
    FlyweightTick* acquire() {
        if (next_index_ >= pool_.size()) {
            return nullptr;
        }
        return &pool_[next_index_++];
    }
    
    // Reset pool for new buffer (invalidates all previous ticks)
    void reset() {
        next_index_ = 0;
    }
    
    // Get all active ticks (valid only while buffer is alive)
    const std::vector<FlyweightTick>& get_ticks() const { return pool_; }
    
    size_t size() const { return next_index_; }
    size_t capacity() const { return pool_.capacity(); }
    bool is_full() const { return next_index_ >= pool_.size(); }

private:
    std::vector<FlyweightTick> pool_;
    size_t next_index_;
};

} // namespace common
} // namespace feedhandler
