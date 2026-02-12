#pragma once

#include <vector>
#include "parser/fsm_fix_parser.hpp"
#include "net/receive_buffer.hpp"
#include "common/tick.hpp"

namespace feedhandler {
namespace parser {

/**
 * @brief Streaming FIX message handler integrating FSM parser with receive buffer
 * 
 * This class demonstrates the complete integration of:
 * - TCP receive buffer (handles fragmentation)
 * - FSM parser (maintains state across buffer boundaries)
 * - Tick output (zero-copy where possible)
 * 
 * Usage pattern:
 * 1. Receive data from socket into buffer
 * 2. Call process_buffer() to parse available data
 * 3. Parser maintains state if message is incomplete
 * 4. Next recv() continues from where it left off
 */
class StreamingFixHandler {
public:
    /**
     * @brief Constructor
     */
    StreamingFixHandler();
    
    /**
     * @brief Process incoming data from socket
     * @param data Raw bytes from recv()
     * @param length Number of bytes received
     * @param ticks Output vector for completed ticks
     * @return Number of ticks parsed
     * 
     * This function:
     * 1. Writes data to receive buffer
     * 2. Parses available data with FSM parser
     * 3. Consumes parsed bytes from buffer
     * 4. Maintains parser state for incomplete messages
     */
    size_t process_incoming_data(const char* data, size_t length, 
                                  std::vector<common::Tick>& ticks);
    
    /**
     * @brief Process data already in the buffer
     * @param ticks Output vector for completed ticks
     * @return Number of ticks parsed
     */
    size_t process_buffer(std::vector<common::Tick>& ticks);
    
    /**
     * @brief Check if handler is currently parsing a message
     */
    bool is_parsing() const { return parser_.is_parsing(); }
    
    /**
     * @brief Get number of bytes in receive buffer
     */
    size_t buffer_bytes() const { return buffer_.readable_bytes(); }
    
    /**
     * @brief Reset handler state (for testing or reconnection)
     */
    void reset();
    
    /**
     * @brief Get statistics
     */
    struct Stats {
        uint64_t total_bytes_received;
        uint64_t total_messages_parsed;
        uint64_t total_parse_calls;
        uint64_t buffer_compactions;
    };
    
    const Stats& get_stats() const { return stats_; }
    
private:
    FSMFixParser parser_;
    net::ReceiveBuffer buffer_;
    Stats stats_;
};

} // namespace parser
} // namespace feedhandler