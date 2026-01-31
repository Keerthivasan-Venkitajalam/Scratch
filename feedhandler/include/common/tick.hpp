#pragma once

#include <string_view>
#include <cstdint>
#include <chrono>

namespace feedhandler {
namespace common {

/**
 * @brief Market data tick representing a single trade or quote event
 * 
 * Designed for zero-copy parsing from FIX protocol messages.
 * The symbol field points directly into the receive buffer.
 * 
 * @warning The symbol string_view must not outlive the source buffer
 */
struct Tick {
    std::string_view symbol;  ///< Instrument symbol (points into buffer)
    int64_t price;           ///< Price in fixed-point (scaled by 10000)
    int32_t qty;             ///< Quantity/size
    char side;               ///< 'B' for Buy/Bid, 'S' for Sell/Ask
    uint64_t timestamp;      ///< Nanoseconds since Unix epoch
    
    /**
     * @brief Default constructor - creates invalid tick
     */
    Tick() : symbol{}, price{0}, qty{0}, side{'\0'}, timestamp{0} {}
    
    /**
     * @brief Constructor with all fields
     */
    Tick(std::string_view sym, int64_t p, int32_t q, char s, uint64_t ts = 0)
        : symbol{sym}, price{p}, qty{q}, side{s}, 
          timestamp{ts == 0 ? current_timestamp_ns() : ts} {}
    
    /**
     * @brief Check if tick is valid
     */
    bool is_valid() const {
        return !symbol.empty() && price > 0 && qty > 0 && 
               (side == 'B' || side == 'S');
    }
    
    /**
     * @brief Get current timestamp in nanoseconds
     */
    static uint64_t current_timestamp_ns() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

/**
 * @brief Convert fixed-point price to double (for display only)
 * @param fixed_price Price scaled by 10000
 * @return Price as double
 */
inline double price_to_double(int64_t fixed_price) {
    return static_cast<double>(fixed_price) / 10000.0;
}

/**
 * @brief Convert double price to fixed-point
 * @param price Price as double
 * @return Price scaled by 10000
 */
inline int64_t double_to_price(double price) {
    return static_cast<int64_t>(price * 10000.0 + 0.5); // +0.5 for rounding
}

/**
 * @brief Convert FIX side value to char
 * @param fix_side FIX tag 54 value: 1=Buy, 2=Sell
 * @return 'B' for buy, 'S' for sell, '\0' for invalid
 */
inline char fix_side_to_char(int fix_side) {
    switch (fix_side) {
        case 1: return 'B';  // Buy
        case 2: return 'S';  // Sell
        default: return '\0';
    }
}

} // namespace common
} // namespace feedhandler