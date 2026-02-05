#include "parser/optimized_fix_parser.hpp"
#include "parser/fast_number_parser.hpp"

#include <chrono>
#include <iostream>
#include <algorithm>

namespace feedhandler {
namespace parser {

common::Tick OptimizedFixParser::parse_message(std::string_view message) {
    // Stack-allocated field storage (no heap allocation)
    constexpr size_t MAX_FIELDS = 32;
    Field fields[MAX_FIELDS];
    
    // Extract all tag-value pairs using optimized parsing
    size_t field_count = extract_fields_optimized(message, fields, MAX_FIELDS);
    
    // Initialize tick with default values
    common::Tick tick;
    
    // Extract required fields for a tick using fast lookups
    // Tag 55: Symbol
    if (const Field* field = find_field_fast(fields, field_count, 55)) {
        tick.symbol = field->value;  // Zero-copy: points directly into input buffer
    }
    
    // Tag 44: Price - use fast fixed-point parsing
    if (const Field* field = find_field_fast(fields, field_count, 44)) {
        tick.price = FastNumberParser::fast_atof_fixed(field->value);
    }
    
    // Tag 38: OrderQty (Quantity) - use fast integer parsing
    if (const Field* field = find_field_fast(fields, field_count, 38)) {
        tick.qty = FastNumberParser::fast_atoi(field->value);
    }
    
    // Tag 54: Side - use fast integer parsing
    if (const Field* field = find_field_fast(fields, field_count, 54)) {
        int side_value = FastNumberParser::fast_atoi(field->value);
        tick.side = common::fix_side_to_char(side_value);
    }
    
    // Set timestamp to current time
    tick.timestamp = common::Tick::current_timestamp_ns();
    
    return tick;
}

std::vector<common::Tick> OptimizedFixParser::parse_messages_from_buffer(std::string_view buffer) {
    std::vector<common::Tick> ticks;
    
    // Split buffer by newlines (assuming each line is a message)
    size_t start = 0;
    size_t pos = 0;
    
    while (pos < buffer.size()) {
        if (buffer[pos] == '\n' || pos == buffer.size() - 1) {
            size_t end = (buffer[pos] == '\n') ? pos : pos + 1;
            if (end > start) {
                std::string_view message = buffer.substr(start, end - start);
                if (!message.empty()) {
                    ticks.push_back(parse_message(message));
                }
            }
            start = pos + 1;
        }
        ++pos;
    }
    
    return ticks;
}

uint64_t OptimizedFixParser::benchmark_parsing(size_t message_count) {
    // Create a sample FIX message for benchmarking
    std::string sample_message = "8=FIX.4.4|9=79|35=D|55=MSFT|44=123.4500|38=1000|54=1|52=20240131-12:34:56|10=020|";
    
    // Create a large buffer with all messages (simulates real network buffer)
    std::string buffer;
    buffer.reserve(sample_message.size() * message_count + message_count);
    
    for (size_t i = 0; i < message_count; ++i) {
        buffer += sample_message;
        if (i < message_count - 1) {
            buffer += '\n';
        }
    }
    
    std::cout << "Benchmarking optimized parser with " << message_count << " messages..." << std::endl;
    
    // Start timing
    auto start = std::chrono::high_resolution_clock::now();
    
    // Parse all messages from the buffer
    auto ticks = parse_messages_from_buffer(buffer);
    
    // End timing
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    uint64_t microseconds = duration.count();
    
    // Calculate statistics
    double messages_per_second = (static_cast<double>(message_count) / microseconds) * 1000000.0;
    double microseconds_per_message = static_cast<double>(microseconds) / message_count;
    
    std::cout << "Optimized Parser Benchmark Results:" << std::endl;
    std::cout << "  Total time: " << microseconds << " μs" << std::endl;
    std::cout << "  Messages parsed: " << message_count << std::endl;
    std::cout << "  Valid ticks: " << std::count_if(ticks.begin(), ticks.end(), 
                                                   [](const auto& t) { return t.is_valid(); }) << std::endl;
    std::cout << "  Messages/second: " << static_cast<uint64_t>(messages_per_second) << std::endl;
    std::cout << "  μs/message: " << microseconds_per_message << std::endl;
    
    return microseconds;
}

size_t OptimizedFixParser::extract_fields_optimized(std::string_view message, Field* fields, size_t max_fields) {
    size_t field_count = 0;
    const char* ptr = message.data();
    const char* end = ptr + message.size();
    
    while (ptr < end && field_count < max_fields) {
        // Find next delimiter '|'
        const char* delim_ptr = ptr;
        while (delim_ptr < end && *delim_ptr != '|') {
            ++delim_ptr;
        }
        
        // Extract field
        if (delim_ptr > ptr) {
            // Find '=' separator
            const char* eq_ptr = ptr;
            while (eq_ptr < delim_ptr && *eq_ptr != '=') {
                ++eq_ptr;
            }
            
            if (eq_ptr < delim_ptr && eq_ptr > ptr) {
                // Parse tag using fast_atoi
                int tag = FastNumberParser::fast_atoi(ptr, eq_ptr);
                if (tag > 0) {
                    fields[field_count].tag = tag;
                    fields[field_count].value = std::string_view(eq_ptr + 1, delim_ptr - eq_ptr - 1);
                    ++field_count;
                }
            }
        }
        
        ptr = delim_ptr + 1;
    }
    
    return field_count;
}

const OptimizedFixParser::Field* OptimizedFixParser::find_field_fast(const Field* fields, size_t field_count, int tag) {
    // Linear search is fast for small arrays (typically < 20 fields)
    // Could be optimized with binary search if field_count is large
    for (size_t i = 0; i < field_count; ++i) {
        if (fields[i].tag == tag) {
            return &fields[i];
        }
    }
    return nullptr;
}

} // namespace parser
} // namespace feedhandler