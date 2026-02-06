#include "parser/repeating_group_parser.hpp"
#include "parser/fast_number_parser.hpp"

#include <chrono>
#include <iostream>
#include <algorithm>

namespace feedhandler {
namespace parser {

std::vector<common::Tick> RepeatingGroupParser::parse_repeating_groups(std::string_view message) {
    std::vector<common::Tick> ticks;
    
    // Stack-allocated field storage
    constexpr size_t MAX_FIELDS = 128;  // Larger for repeating groups
    Field fields[MAX_FIELDS];
    
    // Extract all fields from the message
    size_t field_count = extract_all_fields(message, fields, MAX_FIELDS);
    
    // Find shared fields (symbol, etc.)
    std::string_view symbol;
    if (const Field* field = find_first_field(fields, field_count, 55)) {
        symbol = field->value;
    }
    
    // Find number of repeating groups (Tag 268: NoMDEntries)
    int num_entries = 0;
    if (const Field* field = find_first_field(fields, field_count, 268)) {
        num_entries = FastNumberParser::fast_atoi(field->value);
    }
    
    // If no explicit count, count the number of MDEntryType (269) tags
    if (num_entries == 0) {
        constexpr size_t MAX_INDICES = 32;
        size_t indices[MAX_INDICES];
        num_entries = find_all_fields(fields, field_count, 269, indices, MAX_INDICES);
    }
    
    if (num_entries == 0) {
        // No repeating groups, parse as single tick
        common::Tick tick;
        tick.symbol = symbol;
        
        // Tag 44 or 270: Price
        if (const Field* field = find_first_field(fields, field_count, 44)) {
            tick.price = FastNumberParser::fast_atof_fixed(field->value);
        } else if (const Field* field = find_first_field(fields, field_count, 270)) {
            tick.price = FastNumberParser::fast_atof_fixed(field->value);
        }
        
        // Tag 38 or 271: Quantity
        if (const Field* field = find_first_field(fields, field_count, 38)) {
            tick.qty = FastNumberParser::fast_atoi(field->value);
        } else if (const Field* field = find_first_field(fields, field_count, 271)) {
            tick.qty = FastNumberParser::fast_atoi(field->value);
        }
        
        // Tag 54 or 269: Side
        if (const Field* field = find_first_field(fields, field_count, 54)) {
            int side_value = FastNumberParser::fast_atoi(field->value);
            tick.side = common::fix_side_to_char(side_value);
        } else if (const Field* field = find_first_field(fields, field_count, 269)) {
            int entry_type = FastNumberParser::fast_atoi(field->value);
            tick.side = (entry_type == 0) ? 'B' : (entry_type == 1) ? 'S' : 'T';
        }
        
        tick.timestamp = common::Tick::current_timestamp_ns();
        
        if (tick.is_valid()) {
            ticks.push_back(tick);
        }
        
        return ticks;
    }
    
    // Parse repeating groups
    // Find all MDEntryType (269), MDEntryPx (270), and MDEntrySize (271) tags
    constexpr size_t MAX_INDICES = 32;
    size_t type_indices[MAX_INDICES];
    size_t price_indices[MAX_INDICES];
    size_t size_indices[MAX_INDICES];
    
    size_t type_count = find_all_fields(fields, field_count, 269, type_indices, MAX_INDICES);
    size_t price_count = find_all_fields(fields, field_count, 270, price_indices, MAX_INDICES);
    size_t size_count = find_all_fields(fields, field_count, 271, size_indices, MAX_INDICES);
    
    // Parse each repeating group entry
    size_t entry_count = std::min({type_count, price_count, size_count});
    ticks.reserve(entry_count);
    
    for (size_t i = 0; i < entry_count; ++i) {
        common::Tick tick;
        tick.symbol = symbol;
        
        // MDEntryType (269): 0=Bid, 1=Offer, 2=Trade
        int entry_type = FastNumberParser::fast_atoi(fields[type_indices[i]].value);
        tick.side = (entry_type == 0) ? 'B' : (entry_type == 1) ? 'S' : 'T';
        
        // MDEntryPx (270): Price
        tick.price = FastNumberParser::fast_atof_fixed(fields[price_indices[i]].value);
        
        // MDEntrySize (271): Quantity
        tick.qty = FastNumberParser::fast_atoi(fields[size_indices[i]].value);
        
        tick.timestamp = common::Tick::current_timestamp_ns();
        
        if (tick.is_valid()) {
            ticks.push_back(tick);
        }
    }
    
    return ticks;
}

std::vector<common::Tick> RepeatingGroupParser::parse_buffer_with_repeating_groups(std::string_view buffer) {
    std::vector<common::Tick> all_ticks;
    
    // Split buffer by newlines
    size_t start = 0;
    size_t pos = 0;
    
    while (pos < buffer.size()) {
        if (buffer[pos] == '\n' || pos == buffer.size() - 1) {
            size_t end = (buffer[pos] == '\n') ? pos : pos + 1;
            if (end > start) {
                std::string_view message = buffer.substr(start, end - start);
                if (!message.empty()) {
                    auto ticks = parse_repeating_groups(message);
                    all_ticks.insert(all_ticks.end(), ticks.begin(), ticks.end());
                }
            }
            start = pos + 1;
        }
        ++pos;
    }
    
    return all_ticks;
}

uint64_t RepeatingGroupParser::benchmark_repeating_groups(size_t message_count, size_t entries_per_message) {
    // Create a sample message with repeating groups
    std::string sample_message = "8=FIX.4.4|35=W|55=MSFT|268=" + std::to_string(entries_per_message) + "|";
    
    // Add repeating group entries
    for (size_t i = 0; i < entries_per_message; ++i) {
        int entry_type = (i % 2);  // Alternate between Bid (0) and Offer (1)
        double price = 100.0 + i * 0.25;
        int qty = 1000 + i * 100;
        
        sample_message += "269=" + std::to_string(entry_type) + "|";
        sample_message += "270=" + std::to_string(price) + "|";
        sample_message += "271=" + std::to_string(qty) + "|";
    }
    
    // Create buffer with all messages
    std::string buffer;
    buffer.reserve(sample_message.size() * message_count + message_count);
    
    for (size_t i = 0; i < message_count; ++i) {
        buffer += sample_message;
        if (i < message_count - 1) {
            buffer += '\n';
        }
    }
    
    std::cout << "Benchmarking repeating group parser:" << std::endl;
    std::cout << "  Messages: " << message_count << std::endl;
    std::cout << "  Entries per message: " << entries_per_message << std::endl;
    std::cout << "  Total ticks expected: " << (message_count * entries_per_message) << std::endl;
    
    // Start timing
    auto start = std::chrono::high_resolution_clock::now();
    
    // Parse all messages
    auto ticks = parse_buffer_with_repeating_groups(buffer);
    
    // End timing
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    uint64_t microseconds = duration.count();
    
    // Calculate statistics
    size_t total_ticks = message_count * entries_per_message;
    double ticks_per_second = (static_cast<double>(total_ticks) / microseconds) * 1000000.0;
    double microseconds_per_tick = static_cast<double>(microseconds) / total_ticks;
    
    std::cout << "Repeating Group Parser Benchmark Results:" << std::endl;
    std::cout << "  Total time: " << microseconds << " μs" << std::endl;
    std::cout << "  Messages parsed: " << message_count << std::endl;
    std::cout << "  Ticks generated: " << ticks.size() << std::endl;
    std::cout << "  Valid ticks: " << std::count_if(ticks.begin(), ticks.end(), 
                                                   [](const auto& t) { return t.is_valid(); }) << std::endl;
    std::cout << "  Ticks/second: " << static_cast<uint64_t>(ticks_per_second) << std::endl;
    std::cout << "  μs/tick: " << microseconds_per_tick << std::endl;
    
    return microseconds;
}

size_t RepeatingGroupParser::extract_all_fields(std::string_view message, Field* fields, size_t max_fields) {
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
                // Parse tag
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

size_t RepeatingGroupParser::find_all_fields(const Field* fields, size_t field_count, int tag, 
                                             size_t* indices, size_t max_indices) {
    size_t count = 0;
    for (size_t i = 0; i < field_count && count < max_indices; ++i) {
        if (fields[i].tag == tag) {
            indices[count++] = i;
        }
    }
    return count;
}

const RepeatingGroupParser::Field* RepeatingGroupParser::find_first_field(const Field* fields, size_t field_count, int tag) {
    for (size_t i = 0; i < field_count; ++i) {
        if (fields[i].tag == tag) {
            return &fields[i];
        }
    }
    return nullptr;
}

} // namespace parser
} // namespace feedhandler