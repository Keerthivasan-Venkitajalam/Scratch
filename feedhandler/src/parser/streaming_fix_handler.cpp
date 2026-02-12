#include "parser/streaming_fix_handler.hpp"
#include <iostream>

namespace feedhandler {
namespace parser {

StreamingFixHandler::StreamingFixHandler() 
    : stats_{0, 0, 0, 0} {
}

size_t StreamingFixHandler::process_incoming_data(const char* data, size_t length, 
                                                   std::vector<common::Tick>& ticks) {
    // Write incoming data to receive buffer
    size_t written = buffer_.write(data, length);
    stats_.total_bytes_received += written;
    
    if (written < length) {
        std::cerr << "Warning: Buffer full, dropped " << (length - written) << " bytes" << std::endl;
    }
    
    // Process available data in buffer
    return process_buffer(ticks);
}

size_t StreamingFixHandler::process_buffer(std::vector<common::Tick>& ticks) {
    size_t initial_tick_count = ticks.size();
    
    // Get readable data from buffer
    const char* data = buffer_.read_ptr();
    size_t available = buffer_.readable_bytes();
    
    if (available == 0) {
        return 0;
    }
    
    // Parse available data with FSM parser
    // Parser maintains state if message is incomplete
    size_t consumed = parser_.parse(data, available, ticks);
    
    // Consume parsed bytes from buffer
    if (consumed > 0) {
        buffer_.consume(consumed);
        
        // Track buffer compaction (happens inside consume when needed)
        if (buffer_.readable_bytes() < available - consumed) {
            stats_.buffer_compactions++;
        }
    }
    
    stats_.total_parse_calls++;
    size_t ticks_parsed = ticks.size() - initial_tick_count;
    stats_.total_messages_parsed += ticks_parsed;
    
    return ticks_parsed;
}

void StreamingFixHandler::reset() {
    parser_.reset();
    buffer_.reset();
    stats_ = {0, 0, 0, 0};
}

} // namespace parser
} // namespace feedhandler