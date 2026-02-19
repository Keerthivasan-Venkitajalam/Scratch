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
     * @brief Enable/disable garbage recovery mode
     * @param enable If true, parser will scan for "8=FIX" to resync after errors
     */
    void set_garbage_recovery(bool enable) { garbage_recovery_enabled_ = enable; }
    
    /**
     * @brief Check if garbage recovery is enabled
     */
    bool is_garbage_recovery_enabled() const { return garbage_recovery_enabled_; }
    
    /**
     * @brief Get statistics about garbage recovery
     */
    struct RecoveryStats {
        size_t error_count;          // Number of parsing errors encountered
        size_t recovery_count;       // Number of successful recoveries
        size_t bytes_skipped;        // Total bytes skipped during recovery
        
        RecoveryStats() : error_count(0), recovery_count(0), bytes_skipped(0) {}
    };
    
    const RecoveryStats& get_recovery_stats() const { return recovery_stats_; }
    void reset_recovery_stats() { recovery_stats_ = RecoveryStats(); }
    
    /**
     * @brief Attempt to recover from parsing error by scanning for "8=FIX"
     * @param buffer Remaining buffer to scan
     * @param length Length of remaining buffer
     * @return Number of bytes to skip to reach potential message start
     */
    size_t attempt_garbage_recovery(const char* buffer, size_t length);
    
    /**
     * @brief Check if we're at the start of a FIX message ("8=FIX")
     * @param buffer Buffer to check
     * @param length Length of buffer
     * @return true if buffer starts with "8=FIX"
     */
    bool is_fix_message_start(const char* buffer, size_t length) const;
    
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
    
    // Garbage recovery
    bool garbage_recovery_enabled_;
    RecoveryStats recovery_stats_;
    
    // Recovery state machine
    enum class RecoveryState {
        SCANNING,           // Scanning for '8'
        FOUND_8,           // Found '8', looking for '='
        FOUND_EQUALS,      // Found '8=', looking for 'F'
        FOUND_F,           // Found '8=F', looking for 'I'
        FOUND_FI,          // Found '8=FI', looking for 'X'
        COMPLETE           // Found '8=FIX', can resume parsing
    };
    
    RecoveryState recovery_state_;
};

} // namespace parser
} // namespace feedhandler