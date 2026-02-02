#pragma once

#include <string>
#include <vector>
#include <map>
#include "common/tick.hpp"

namespace feedhandler {
namespace parser {

/**
 * @brief Naive FIX parser using std::string and std::stringstream
 * 
 * This is the baseline implementation for performance comparison.
 * Uses standard library components with heap allocations.
 * 
 * Performance characteristics:
 * - Multiple heap allocations per message
 * - String copying for each field
 * - Stream-based parsing overhead
 */
class NaiveFixParser {
public:
    /**
     * @brief Parse a single FIX message into a Tick
     * @param message FIX message string (SOH replaced with '|' for readability)
     * @return Parsed tick, or invalid tick if parsing fails
     */
    static common::Tick parse_message(const std::string& message);
    
    /**
     * @brief Parse multiple FIX messages
     * @param messages Vector of FIX message strings
     * @return Vector of parsed ticks
     */
    static std::vector<common::Tick> parse_messages(const std::vector<std::string>& messages);
    
    /**
     * @brief Benchmark parsing performance
     * @param message_count Number of messages to parse
     * @return Parsing time in microseconds
     */
    static uint64_t benchmark_parsing(size_t message_count);

private:
    /**
     * @brief Extract tag-value pairs from FIX message
     * @param message FIX message string
     * @return Map of tag -> value
     */
    static std::map<int, std::string> extract_fields(const std::string& message);
    
    /**
     * @brief Convert string to integer safely
     * @param str String to convert
     * @param default_value Default value if conversion fails
     * @return Converted integer or default value
     */
    static int safe_stoi(const std::string& str, int default_value = 0);
    
    /**
     * @brief Convert string to double safely
     * @param str String to convert
     * @param default_value Default value if conversion fails
     * @return Converted double or default value
     */
    static double safe_stod(const std::string& str, double default_value = 0.0);
};

} // namespace parser
} // namespace feedhandler