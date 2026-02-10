#pragma once

#include <string_view>
#include <vector>
#include <cstdint>
#include "common/tick.hpp"

namespace feedhandler {
namespace parser {

/**
 * @brief Finite State Machine FIX parser for streaming data
 * 
 * This parser can handle fragmented TCP streams by maintaining state
 * between parse calls. It processes input character-by-character and
 * can stop mid-message and resume later.
 * 
 * Performance characteristics:
 * - Streaming capable (handles fragmented messages)
 * - Zero heap allocations during parsing
 * - State preserved between calls
 * - Optimized for high-frequency parsing
 */
class FSMFixParser {
public:
    /**
     * @brief Parser state machine states
     */
    enum class State {
        WAIT_TAG,       // Waiting for start of tag
        READ_TAG,       // Reading tag digits
        WAIT_VALUE,     // Waiting for '=' separator
        READ_VALUE,     // Reading value characters
        COMPLETE        // Message complete
    };
    
    /**
     * @brief Constructor
     */
    FSMFixParser();
    
    /**
     * @brief Reset parser state to start fresh
     */
    void reset();
    
    /**
     * @brief Parse input buffer character-by-character
     * @param buffer Input buffer to parse
     * @param length Length of input buffer
     * @param ticks Output vector for completed ticks
     * @return Number of bytes consumed from buffer
     * 
     * This function can be called multiple times with fragmented data.
     * It will resume from where it left off and emit ticks as messages complete.
     */
    size_t parse(const char* buffer, size_t length, std::vector<common::Tick>& ticks);
    
    /**
     * @brief Check if parser is currently in the middle of a message
     */
    bool is_parsing() const { return state_ != State::WAIT_TAG || current_tag_ != 0; }
    
    /**
     * @brief Get current parser state (for debugging)
     */
    State get_state() const { return state_; }
    
    /**
     * @brief Benchmark parsing performance with streaming data
     * @param message_count Number of messages to parse
     * @return Parsing time in microseconds
     */
    static uint64_t benchmark_parsing(size_t message_count);

private:
    /**
     * @brief Process a single character through the state machine
     * @param c Character to process
     * @return true if a message was completed
     */
    bool process_char(char c);
    
    /**
     * @brief Finalize current message and create tick
     */
    void finalize_message();
    
    /**
     * @brief Parse integer from accumulated digits
     */
    int parse_accumulated_int() const;
    
    /**
     * @brief Parse double from accumulated digits
     */
    double parse_accumulated_double() const;
    
    // Parser state
    State state_;
    
    // Current field being parsed
    int current_tag_;
    char value_buffer_[256];  // Buffer for current value
    size_t value_length_;
    
    // Accumulated tag digits
    char tag_buffer_[16];
    size_t tag_length_;
    
    // Current tick being built
    struct TickBuilder {
        char symbol_storage[64];  // Persistent storage for symbol
        size_t symbol_length;
        int64_t price;
        int32_t qty;
        char side;
        bool has_symbol;
        bool has_price;
        bool has_qty;
        bool has_side;
        
        TickBuilder() : symbol_length(0), price(0), qty(0), side('\0'), 
                       has_symbol(false), has_price(false), 
                       has_qty(false), has_side(false) {}
        
        void reset() {
            symbol_length = 0;
            price = 0;
            qty = 0;
            side = '\0';
            has_symbol = has_price = has_qty = has_side = false;
        }
        
        bool is_valid() const {
            return has_symbol && has_price && has_qty && has_side;
        }
        
        std::string_view get_symbol() const {
            return std::string_view(symbol_storage, symbol_length);
        }
    };
    
    TickBuilder tick_builder_;
    
    // Symbol storage for zero-copy (points into value_buffer_)
    size_t symbol_start_;
    size_t symbol_length_;
};

} // namespace parser
} // namespace feedhandler