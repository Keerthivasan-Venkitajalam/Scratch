#include "parser/fsm_fix_parser.hpp"
#include "parser/fast_number_parser.hpp"

#include <chrono>
#include <iostream>
#include <cstring>

namespace feedhandler {
namespace parser {

FSMFixParser::FSMFixParser() 
    : state_(State::WAIT_TAG)
    , current_tag_(0)
    , value_length_(0)
    , tag_length_(0)
    , symbol_start_(0)
    , symbol_length_(0) {
}

void FSMFixParser::reset() {
    state_ = State::WAIT_TAG;
    current_tag_ = 0;
    value_length_ = 0;
    tag_length_ = 0;
    symbol_start_ = 0;
    symbol_length_ = 0;
    tick_builder_.reset();
}

size_t FSMFixParser::parse(const char* buffer, size_t length, std::vector<common::Tick>& ticks) {
    size_t consumed = 0;
    
    for (size_t i = 0; i < length; ++i) {
        char c = buffer[i];
        
        if (process_char(c)) {
            // Message complete - create tick
            finalize_message();
            
            if (tick_builder_.is_valid()) {
                common::Tick tick;
                tick.symbol = tick_builder_.get_symbol();
                tick.price = tick_builder_.price;
                tick.qty = tick_builder_.qty;
                tick.side = tick_builder_.side;
                tick.timestamp = common::Tick::current_timestamp_ns();
                
                ticks.push_back(tick);
            }
            
            // Reset for next message
            tick_builder_.reset();
        }
        
        consumed = i + 1;
    }
    
    return consumed;
}

bool FSMFixParser::process_char(char c) {
    switch (state_) {
        case State::WAIT_TAG:
            if (c >= '0' && c <= '9') {
                // Start of new tag
                tag_buffer_[0] = c;
                tag_length_ = 1;
                state_ = State::READ_TAG;
            }
            // Ignore whitespace and delimiters
            break;
            
        case State::READ_TAG:
            if (c >= '0' && c <= '9') {
                // Continue reading tag
                if (tag_length_ < sizeof(tag_buffer_) - 1) {
                    tag_buffer_[tag_length_++] = c;
                }
            } else if (c == '=') {
                // End of tag, parse it
                tag_buffer_[tag_length_] = '\0';
                current_tag_ = parse_accumulated_int();
                value_length_ = 0;
                state_ = State::READ_VALUE;
            } else {
                // Invalid character, reset
                state_ = State::WAIT_TAG;
                current_tag_ = 0;
                tag_length_ = 0;
            }
            break;
        
        case State::WAIT_VALUE:
            // Waiting for value to start (not used in current implementation)
            // Transition directly to READ_VALUE
            state_ = State::READ_VALUE;
            // Fall through to process this character as value
            [[fallthrough]];
            
        case State::READ_VALUE:
            if (c == '|' || c == '\x01' || c == '\n' || c == '\r') {
                // End of value - process the field using optimized tag switch
                value_buffer_[value_length_] = '\0';
                
                // Optimized tag switch for O(1) field assignment
                // Compiler will generate a jump table for dense tag ranges
                switch (current_tag_) {
                    case 38: // OrderQty (Quantity) - HOT PATH
                        tick_builder_.qty = parse_accumulated_int();
                        tick_builder_.has_qty = true;
                        break;
                        
                    case 44: // Price - HOT PATH
                        {
                            double price_double = parse_accumulated_double();
                            tick_builder_.price = common::double_to_price(price_double);
                            tick_builder_.has_price = true;
                        }
                        break;
                        
                    case 54: // Side - HOT PATH
                        {
                            int side_value = parse_accumulated_int();
                            tick_builder_.side = common::fix_side_to_char(side_value);
                            tick_builder_.has_side = true;
                        }
                        break;
                        
                    case 55: // Symbol - HOT PATH
                        // Copy symbol to persistent storage
                        if (value_length_ < sizeof(tick_builder_.symbol_storage)) {
                            std::memcpy(tick_builder_.symbol_storage, value_buffer_, value_length_);
                            tick_builder_.symbol_length = value_length_;
                            tick_builder_.has_symbol = true;
                        }
                        break;
                        
                    case 10: // Checksum - end of message
                        state_ = State::COMPLETE;
                        current_tag_ = 0;
                        return true; // Message complete
                        
                    // Less common tags - grouped together
                    case 8:  // BeginString
                    case 9:  // BodyLength
                    case 35: // MsgType
                    case 52: // SendingTime
                    default:
                        // Ignore unknown/unneeded tags
                        break;
                }
                
                // Reset for next field
                current_tag_ = 0;
                value_length_ = 0;
                state_ = State::WAIT_TAG;
                
                // Check if we have a complete tick (even without checksum)
                if (c == '\n' && tick_builder_.is_valid()) {
                    state_ = State::COMPLETE;
                    return true;
                }
            } else {
                // Accumulate value character
                if (value_length_ < sizeof(value_buffer_) - 1) {
                    value_buffer_[value_length_++] = c;
                }
            }
            break;
            
        case State::COMPLETE:
            // Message was complete, reset to start new one
            state_ = State::WAIT_TAG;
            current_tag_ = 0;
            
            // Process this character as start of new message
            if (c >= '0' && c <= '9') {
                tag_buffer_[0] = c;
                tag_length_ = 1;
                state_ = State::READ_TAG;
            }
            break;
    }
    
    return false;
}

void FSMFixParser::finalize_message() {
    // Ensure value buffer is null-terminated
    if (value_length_ < sizeof(value_buffer_)) {
        value_buffer_[value_length_] = '\0';
    }
    
    state_ = State::WAIT_TAG;
}

int FSMFixParser::parse_accumulated_int() const {
    if (state_ == State::READ_TAG) {
        // Parse from tag_buffer_
        return FastNumberParser::fast_atoi(tag_buffer_, tag_buffer_ + tag_length_);
    } else {
        // Parse from value_buffer_
        return FastNumberParser::fast_atoi(value_buffer_, value_buffer_ + value_length_);
    }
}

double FSMFixParser::parse_accumulated_double() const {
    // Parse from value_buffer_ using fixed-point conversion
    int64_t fixed_value = FastNumberParser::fast_atof_fixed(value_buffer_, value_buffer_ + value_length_);
    // Convert back to double for compatibility
    return static_cast<double>(fixed_value) / 10000.0;
}

uint64_t FSMFixParser::benchmark_parsing(size_t message_count) {
    // Create a sample FIX message for benchmarking
    std::string sample_message = "8=FIX.4.4|9=79|35=D|55=MSFT|44=123.4500|38=1000|54=1|52=20240131-12:34:56|10=020|\n";
    
    // Create a large buffer with all messages
    std::string buffer;
    buffer.reserve(sample_message.size() * message_count);
    
    for (size_t i = 0; i < message_count; ++i) {
        buffer += sample_message;
    }
    
    std::cout << "Benchmarking FSM parser with " << message_count << " messages..." << std::endl;
    
    // Create parser
    FSMFixParser parser;
    std::vector<common::Tick> ticks;
    ticks.reserve(message_count);
    
    // Start timing
    auto start = std::chrono::high_resolution_clock::now();
    
    // Parse all messages (simulating streaming by parsing in chunks)
    const size_t chunk_size = 1024;
    size_t offset = 0;
    
    while (offset < buffer.size()) {
        size_t chunk_len = std::min(chunk_size, buffer.size() - offset);
        size_t consumed = parser.parse(buffer.data() + offset, chunk_len, ticks);
        offset += consumed;
    }
    
    // End timing
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    uint64_t microseconds = duration.count();
    
    // Calculate statistics
    double messages_per_second = (static_cast<double>(message_count) / microseconds) * 1000000.0;
    double microseconds_per_message = static_cast<double>(microseconds) / message_count;
    
    std::cout << "FSM Parser Benchmark Results:" << std::endl;
    std::cout << "  Total time: " << microseconds << " μs" << std::endl;
    std::cout << "  Messages parsed: " << message_count << std::endl;
    std::cout << "  Valid ticks: " << ticks.size() << std::endl;
    std::cout << "  Messages/second: " << static_cast<uint64_t>(messages_per_second) << std::endl;
    std::cout << "  μs/message: " << microseconds_per_message << std::endl;
    
    return microseconds;
}

} // namespace parser
} // namespace feedhandler