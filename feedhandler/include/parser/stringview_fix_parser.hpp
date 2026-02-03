#pragma once

#include <string_view>
#include <vector>
#include <array>
#include "common/tick.hpp"

namespace feedhandler {
namespace parser {

/**
 * @brief Zero-allocation FIX parser using std::string_view
 * 
 * This parser eliminates heap allocations by using string_view
 * to reference fields directly in the input buffer.
 * 
 * Performance characteristics:
 * - Zero heap allocations during parsing
 * - Fields are views into the original buffer
 * - Requires buffer lifetime management
 * - 2-3x faster than naive parser
 */
class StringViewFixParser {
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
     * @brief Extract tag-value pairs from FIX message using string_view
     * @param message FIX message string_view
     * @param fields Output array for fields (must be large enough)
     * @return Number of fields extracted
     */
    static size_t extract_fields(std::string_view message, Field* fields, size_t max_fields);
    
    /**
     * @brief Find field by tag in the fields array
     * @param fields Array of fields
     * @param field_count Number of fields in array
     * @param tag Tag to search for
     * @return Pointer to field if found, nullptr otherwise
     */
    static const Field* find_field(const Field* fields, size_t field_count, int tag);
    
    /**
     * @brief Convert string_view to integer safely (no exceptions)
     * @param str String view to convert
     * @param default_value Default value if conversion fails
     * @return Converted integer or default value
     */
    static int safe_sv_to_int(std::string_view str, int default_value = 0);
    
    /**
     * @brief Convert string_view to double safely (no exceptions)
     * @param str String view to convert
     * @param default_value Default value if conversion fails
     * @return Converted double or default value
     */
    static double safe_sv_to_double(std::string_view str, double default_value = 0.0);
    
    /**
     * @brief Parse integer from string_view manually (faster than std::stoi)
     * @param str String view containing integer
     * @return Parsed integer, or 0 if invalid
     */
    static int parse_int(std::string_view str);
    
    /**
     * @brief Parse double from string_view manually (faster than std::stod)
     * @param str String view containing double
     * @return Parsed double, or 0.0 if invalid
     */
    static double parse_double(std::string_view str);
};

} // namespace parser
} // namespace feedhandler