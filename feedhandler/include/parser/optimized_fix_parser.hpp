#pragma once

#include <string_view>
#include <vector>
#include "common/tick.hpp"

namespace feedhandler {
namespace parser {

/**
 * @brief Optimized FIX parser combining string_view with fast number parsing
 * 
 * This parser builds on the string_view parser by adding optimized
 * number parsing functions for maximum performance.
 * 
 * Performance characteristics:
 * - Zero heap allocations during parsing
 * - Fast custom number parsing (no std::stoi/stod)
 * - Optimized for FIX protocol field parsing
 * - Expected 20-30% improvement over string_view parser
 */
class OptimizedFixParser {
public:
    /**
     * @brief Parse a single FIX message into a Tick
     * @param message FIX message string_view (SOH replaced with '|' for readability)
     * @return Parsed tick, or invalid tick if parsing fails
     * @warning The input buffer must remain valid for the lifetime of the returned Tick
     */
    static common::Tick parse_message(std::string_view message);
    
    /**
     * @brief Parse multiple FIX messages from a single buffer
     * @param buffer Buffer containing multiple messages separated by newlines
     * @return Vector of parsed ticks
     * @warning The input buffer must remain valid for the lifetime of all returned Ticks
     */
    static std::vector<common::Tick> parse_messages_from_buffer(std::string_view buffer);
    
    /**
     * @brief Benchmark parsing performance
     * @param message_count Number of messages to parse
     * @return Parsing time in microseconds
     */
    static uint64_t benchmark_parsing(size_t message_count);

private:
    /**
     * @brief Field storage for tag-value pairs
     */
    struct Field {
        int tag;
        std::string_view value;
    };
    
    /**
     * @brief Extract tag-value pairs from FIX message using optimized parsing
     * @param message FIX message string_view
     * @param fields Output array for fields (must be large enough)
     * @param max_fields Maximum number of fields to extract
     * @return Number of fields extracted
     */
    static size_t extract_fields_optimized(std::string_view message, Field* fields, size_t max_fields);
    
    /**
     * @brief Find field by tag in the fields array (optimized)
     * @param fields Array of fields
     * @param field_count Number of fields in array
     * @param tag Tag to search for
     * @return Pointer to field if found, nullptr otherwise
     */
    static const Field* find_field_fast(const Field* fields, size_t field_count, int tag);
};

} // namespace parser
} // namespace feedhandler