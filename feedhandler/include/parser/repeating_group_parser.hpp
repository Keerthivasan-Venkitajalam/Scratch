#pragma once

#include <string_view>
#include <vector>
#include "common/tick.hpp"

namespace feedhandler {
namespace parser {

/**
 * @brief Parser for FIX messages with repeating groups
 * 
 * Handles FIX messages that contain multiple repeating entries,
 * such as market data snapshots with multiple price levels or
 * execution reports with multiple fills.
 * 
 * Example: A market data message with 3 price levels:
 *   8=FIX.4.4|268=3|269=0|270=100.50|271=1000|269=0|270=100.25|271=500|269=1|270=100.75|271=750|
 *   
 * This would be parsed into 3 separate Tick objects.
 */
class RepeatingGroupParser {
public:
    /**
     * @brief Parse a FIX message with repeating groups into multiple Ticks
     * @param message FIX message string_view
     * @return Vector of parsed ticks
     * @warning The input buffer must remain valid for the lifetime of all returned Ticks
     * 
     * Repeating group format:
     * - Tag 268: NoMDEntries (number of repeating groups)
     * - Tag 269: MDEntryType (0=Bid, 1=Offer, 2=Trade)
     * - Tag 270: MDEntryPx (price)
     * - Tag 271: MDEntrySize (quantity)
     * - Tag 55: Symbol (shared across all entries)
     */
    static std::vector<common::Tick> parse_repeating_groups(std::string_view message);
    
    /**
     * @brief Parse multiple messages with repeating groups from a buffer
     * @param buffer Buffer containing multiple messages separated by newlines
     * @return Vector of all parsed ticks from all messages
     */
    static std::vector<common::Tick> parse_buffer_with_repeating_groups(std::string_view buffer);
    
    /**
     * @brief Benchmark parsing performance with repeating groups
     * @param message_count Number of messages to parse
     * @param entries_per_message Number of repeating entries per message
     * @return Parsing time in microseconds
     */
    static uint64_t benchmark_repeating_groups(size_t message_count, size_t entries_per_message);

private:
    /**
     * @brief Field storage for tag-value pairs
     */
    struct Field {
        int tag;
        std::string_view value;
    };
    
    /**
     * @brief Extract all fields from message
     * @param message FIX message string_view
     * @param fields Output array for fields
     * @param max_fields Maximum number of fields
     * @return Number of fields extracted
     */
    static size_t extract_all_fields(std::string_view message, Field* fields, size_t max_fields);
    
    /**
     * @brief Find all occurrences of a tag in the fields array
     * @param fields Array of fields
     * @param field_count Number of fields in array
     * @param tag Tag to search for
     * @param indices Output array for field indices
     * @param max_indices Maximum number of indices to store
     * @return Number of occurrences found
     */
    static size_t find_all_fields(const Field* fields, size_t field_count, int tag, 
                                   size_t* indices, size_t max_indices);
    
    /**
     * @brief Find first occurrence of a tag
     * @param fields Array of fields
     * @param field_count Number of fields in array
     * @param tag Tag to search for
     * @return Pointer to field if found, nullptr otherwise
     */
    static const Field* find_first_field(const Field* fields, size_t field_count, int tag);
};

} // namespace parser
} // namespace feedhandler