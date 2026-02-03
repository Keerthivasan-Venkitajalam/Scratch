#include "parser/stringview_fix_parser.hpp"

#include <chrono>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace feedhandler {
namespace parser {

common::Tick StringViewFixParser::parse_message(std::string_view message) {
    // Stack-allocated field storage (no heap allocation)
    constexpr size_t MAX_FIELDS = 32;
    Field fields[MAX_FIELDS];
    
    // Extract all tag-value pairs
    size_t field_count = extract_fields(message, fields, MAX_FIELDS);
    
    // Initialize tick with default values
    common::Tick tick;
    
    // Extract required fields for a tick
    // Tag 55: Symbol
    if (const Field* field = find_field(fields, field_count, 55)) {
        tick.symbol = field->value;  // Zero-copy: points directly into input buffer
    }
    
    // Tag 44: Price
    if (const Field* field = find_field(fields, field_count, 44)) {
        double price_double = parse_double(field->value);
        tick.price = common::double_to_price(price_double);
    }
    
    // Tag 38: OrderQty (Quantity)
    if (const Field* field = find_field(fields, field_count, 38)) {
        tick.qty = parse_int(field->value);
    }
    
    // Tag 54: Side
    if (const Field* field = find_field(fields, field_count, 54)) {
        int side_value = parse_int(field->value);
        tick.side = common::fix_side_to_char(side_value);
    }
    
    // Set timestamp to current time
    tick.timestamp = common::Tick::current_timestamp_ns();
    
    return tick;
}

std::vector<common::Tick> StringViewFixParser::parse_messages_from_buffer(std::string_view buffer) {
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

uint64_t StringViewFixParser::benchmark_parsing(size_t message_count) {
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
    
    std::cout << "Benchmarking string_view parser with " << message_count << " messages..." << std::endl;
    
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
    
    std::cout << "String_view Parser Benchmark Results:" << std::endl;
    std::cout << "  Total time: " << microseconds << " μs" << std::endl;
    std::cout << "  Messages parsed: " << message_count << std::endl;
    std::cout << "  Valid ticks: " << std::count_if(ticks.begin(), ticks.end(), 
                                                   [](const auto& t) { return t.is_valid(); }) << std::endl;
    std::cout << "  Messages/second: " << static_cast<uint64_t>(messages_per_second) << std::endl;
    std::cout << "  μs/message: " << microseconds_per_message << std::endl;
    
    return microseconds;
}

size_t StringViewFixParser::extract_fields(std::string_view message, Field* fields, size_t max_fields) {
    size_t field_count = 0;
    size_t pos = 0;
    
    while (pos < message.size() && field_count < max_fields) {
        // Find next delimiter '|'
        size_t delim_pos = message.find('|', pos);
        if (delim_pos == std::string_view::npos) {
            delim_pos = message.size();
        }
        
        // Extract field
        std::string_view field = message.substr(pos, delim_pos - pos);
        if (!field.empty()) {
            // Find '=' separator
            size_t eq_pos = field.find('=');
            if (eq_pos != std::string_view::npos && eq_pos > 0) {
                // Extract tag and value
                std::string_view tag_str = field.substr(0, eq_pos);
                std::string_view value = field.substr(eq_pos + 1);
                
                // Parse tag
                int tag = parse_int(tag_str);
                if (tag > 0) {
                    fields[field_count].tag = tag;
                    fields[field_count].value = value;
                    ++field_count;
                }
            }
        }
        
        pos = delim_pos + 1;
    }
    
    return field_count;
}

const StringViewFixParser::Field* StringViewFixParser::find_field(const Field* fields, size_t field_count, int tag) {
    for (size_t i = 0; i < field_count; ++i) {
        if (fields[i].tag == tag) {
            return &fields[i];
        }
    }
    return nullptr;
}

int StringViewFixParser::safe_sv_to_int(std::string_view str, int default_value) {
    int result = parse_int(str);
    return (result == 0 && !str.empty() && str[0] != '0') ? default_value : result;
}

double StringViewFixParser::safe_sv_to_double(std::string_view str, double default_value) {
    double result = parse_double(str);
    return (result == 0.0 && !str.empty() && str[0] != '0') ? default_value : result;
}

int StringViewFixParser::parse_int(std::string_view str) {
    if (str.empty()) return 0;
    
    int result = 0;
    size_t i = 0;
    bool negative = false;
    
    // Handle sign
    if (str[0] == '-') {
        negative = true;
        i = 1;
    } else if (str[0] == '+') {
        i = 1;
    }
    
    // Parse digits
    for (; i < str.size(); ++i) {
        char c = str[i];
        if (c >= '0' && c <= '9') {
            result = result * 10 + (c - '0');
        } else {
            break; // Stop at first non-digit
        }
    }
    
    return negative ? -result : result;
}

double StringViewFixParser::parse_double(std::string_view str) {
    if (str.empty()) return 0.0;
    
    double result = 0.0;
    size_t i = 0;
    bool negative = false;
    
    // Handle sign
    if (str[0] == '-') {
        negative = true;
        i = 1;
    } else if (str[0] == '+') {
        i = 1;
    }
    
    // Parse integer part
    for (; i < str.size(); ++i) {
        char c = str[i];
        if (c >= '0' && c <= '9') {
            result = result * 10.0 + (c - '0');
        } else if (c == '.') {
            ++i;
            break;
        } else {
            break; // Stop at first invalid character
        }
    }
    
    // Parse fractional part
    double fraction = 0.1;
    for (; i < str.size(); ++i) {
        char c = str[i];
        if (c >= '0' && c <= '9') {
            result += (c - '0') * fraction;
            fraction *= 0.1;
        } else {
            break; // Stop at first non-digit
        }
    }
    
    return negative ? -result : result;
}

} // namespace parser
} // namespace feedhandler