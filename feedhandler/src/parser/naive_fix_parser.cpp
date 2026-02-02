#include "parser/naive_fix_parser.hpp"

#include <sstream>
#include <map>
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace feedhandler {
namespace parser {

common::Tick NaiveFixParser::parse_message(const std::string& message) {
    // Extract all tag-value pairs
    auto fields = extract_fields(message);
    
    // Initialize tick with default values
    common::Tick tick;
    
    // For single message parsing, we'll use a static storage
    // This is a limitation of the naive approach
    static std::string symbol_storage;
    
    // Extract required fields for a tick
    // Tag 55: Symbol
    if (fields.find(55) != fields.end()) {
        symbol_storage = fields[55];
        tick.symbol = std::string_view(symbol_storage);
    }
    
    // Tag 44: Price
    if (fields.find(44) != fields.end()) {
        double price_double = safe_stod(fields[44]);
        tick.price = common::double_to_price(price_double);
    }
    
    // Tag 38: OrderQty (Quantity)
    if (fields.find(38) != fields.end()) {
        tick.qty = safe_stoi(fields[38]);
    }
    
    // Tag 54: Side
    if (fields.find(54) != fields.end()) {
        int side_value = safe_stoi(fields[54]);
        tick.side = common::fix_side_to_char(side_value);
    }
    
    // Set timestamp to current time
    tick.timestamp = common::Tick::current_timestamp_ns();
    
    return tick;
}

std::vector<common::Tick> NaiveFixParser::parse_messages(const std::vector<std::string>& messages) {
    std::vector<common::Tick> ticks;
    ticks.reserve(messages.size());
    
    // Storage for symbols to ensure string_view validity
    static std::vector<std::string> symbol_storage;
    symbol_storage.clear();
    symbol_storage.reserve(messages.size());
    
    for (size_t i = 0; i < messages.size(); ++i) {
        auto fields = extract_fields(messages[i]);
        
        common::Tick tick;
        
        // Tag 55: Symbol - store in our symbol storage
        if (fields.find(55) != fields.end()) {
            symbol_storage.push_back(fields[55]);
            tick.symbol = std::string_view(symbol_storage.back());
        }
        
        // Tag 44: Price
        if (fields.find(44) != fields.end()) {
            double price_double = safe_stod(fields[44]);
            tick.price = common::double_to_price(price_double);
        }
        
        // Tag 38: OrderQty (Quantity)
        if (fields.find(38) != fields.end()) {
            tick.qty = safe_stoi(fields[38]);
        }
        
        // Tag 54: Side
        if (fields.find(54) != fields.end()) {
            int side_value = safe_stoi(fields[54]);
            tick.side = common::fix_side_to_char(side_value);
        }
        
        // Set timestamp to current time
        tick.timestamp = common::Tick::current_timestamp_ns();
        
        ticks.push_back(tick);
    }
    
    return ticks;
}

uint64_t NaiveFixParser::benchmark_parsing(size_t message_count) {
    // Create a sample FIX message for benchmarking
    std::string sample_message = "8=FIX.4.4|9=79|35=D|55=MSFT|44=123.4500|38=1000|54=1|52=20240131-12:34:56|10=020|";
    
    // Create vector of messages
    std::vector<std::string> messages(message_count, sample_message);
    
    std::cout << "Benchmarking naive parser with " << message_count << " messages..." << std::endl;
    
    // Start timing
    auto start = std::chrono::high_resolution_clock::now();
    
    // Parse all messages
    auto ticks = parse_messages(messages);
    
    // End timing
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    uint64_t microseconds = duration.count();
    
    // Calculate statistics
    double messages_per_second = (static_cast<double>(message_count) / microseconds) * 1000000.0;
    double microseconds_per_message = static_cast<double>(microseconds) / message_count;
    
    std::cout << "Naive Parser Benchmark Results:" << std::endl;
    std::cout << "  Total time: " << microseconds << " μs" << std::endl;
    std::cout << "  Messages parsed: " << message_count << std::endl;
    std::cout << "  Valid ticks: " << std::count_if(ticks.begin(), ticks.end(), 
                                                   [](const auto& t) { return t.is_valid(); }) << std::endl;
    std::cout << "  Messages/second: " << static_cast<uint64_t>(messages_per_second) << std::endl;
    std::cout << "  μs/message: " << microseconds_per_message << std::endl;
    
    return microseconds;
}

std::map<int, std::string> NaiveFixParser::extract_fields(const std::string& message) {
    std::map<int, std::string> fields;
    std::stringstream ss(message);
    std::string field;
    
    // Split by '|' (representing SOH in readable format)
    while (std::getline(ss, field, '|')) {
        if (field.empty()) continue;
        
        // Find the '=' separator
        size_t eq_pos = field.find('=');
        if (eq_pos == std::string::npos) continue;
        
        // Extract tag and value
        std::string tag_str = field.substr(0, eq_pos);
        std::string value = field.substr(eq_pos + 1);
        
        // Convert tag to integer
        int tag = safe_stoi(tag_str);
        if (tag > 0) {
            fields[tag] = value;
        }
    }
    
    return fields;
}

int NaiveFixParser::safe_stoi(const std::string& str, int default_value) {
    try {
        return std::stoi(str);
    } catch (const std::exception&) {
        return default_value;
    }
}

double NaiveFixParser::safe_stod(const std::string& str, double default_value) {
    try {
        return std::stod(str);
    } catch (const std::exception&) {
        return default_value;
    }
}

} // namespace parser
} // namespace feedhandler