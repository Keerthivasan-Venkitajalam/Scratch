#pragma once

#include <cstdint>
#include <string_view>

namespace feedhandler {
namespace parser {

/**
 * @brief High-performance number parsing functions
 * 
 * These functions are optimized for parsing FIX protocol numeric fields
 * with minimal overhead and no exception handling.
 */
class FastNumberParser {
public:
    /**
     * @brief Fast integer parsing from character range
     * @param begin Pointer to first character
     * @param end Pointer to one past last character
     * @return Parsed integer, 0 if invalid
     * 
     * Features:
     * - No exceptions thrown
     * - Handles positive and negative numbers
     * - Stops at first non-digit character
     * - Overflow protection
     */
    static inline int fast_atoi(const char* begin, const char* end);
    
    /**
     * @brief Fast integer parsing from string_view
     * @param str String view to parse
     * @return Parsed integer, 0 if invalid
     */
    static inline int fast_atoi(std::string_view str);
    
    /**
     * @brief Fast fixed-point decimal parsing (no floating point)
     * @param begin Pointer to first character
     * @param end Pointer to one past last character
     * @param scale Scaling factor (e.g., 10000 for 4 decimal places)
     * @return Parsed value scaled by scale factor
     * 
     * Example: "123.4567" with scale=10000 → 1234567
     * 
     * Features:
     * - No floating-point arithmetic
     * - Handles decimal point
     * - Leading zeros supported
     * - No exceptions
     */
    static inline int64_t fast_atof_fixed(const char* begin, const char* end, int64_t scale = 10000);
    
    /**
     * @brief Fast fixed-point decimal parsing from string_view
     * @param str String view to parse
     * @param scale Scaling factor (default: 10000 for 4 decimal places)
     * @return Parsed value scaled by scale factor
     */
    static inline int64_t fast_atof_fixed(std::string_view str, int64_t scale = 10000);
    
    /**
     * @brief Parse unsigned integer (optimized for positive numbers)
     * @param begin Pointer to first character
     * @param end Pointer to one past last character
     * @return Parsed unsigned integer, 0 if invalid
     */
    static inline uint32_t fast_atou(const char* begin, const char* end);
    
    /**
     * @brief Parse unsigned integer from string_view
     * @param str String view to parse
     * @return Parsed unsigned integer, 0 if invalid
     */
    static inline uint32_t fast_atou(std::string_view str);

private:
    /**
     * @brief Check if character is a digit
     * @param c Character to check
     * @return true if digit (0-9)
     */
    static inline bool is_digit(char c) {
        return c >= '0' && c <= '9';
    }
    
    /**
     * @brief Convert digit character to integer
     * @param c Digit character
     * @return Integer value (0-9)
     */
    static inline int digit_to_int(char c) {
        return c - '0';
    }
};

// Inline implementations for maximum performance

inline int FastNumberParser::fast_atoi(const char* begin, const char* end) {
    if (begin >= end) return 0;
    
    bool negative = false;
    const char* ptr = begin;
    
    // Handle sign
    if (*ptr == '-') {
        negative = true;
        ++ptr;
    } else if (*ptr == '+') {
        ++ptr;
    }
    
    if (ptr >= end) return 0;
    
    int result = 0;
    constexpr int MAX_SAFE = INT32_MAX / 10;
    
    while (ptr < end && is_digit(*ptr)) {
        // Overflow protection
        if (result > MAX_SAFE) {
            return negative ? INT32_MIN : INT32_MAX;
        }
        
        result = result * 10 + digit_to_int(*ptr);
        ++ptr;
    }
    
    return negative ? -result : result;
}

inline int FastNumberParser::fast_atoi(std::string_view str) {
    return fast_atoi(str.data(), str.data() + str.size());
}

inline int64_t FastNumberParser::fast_atof_fixed(const char* begin, const char* end, int64_t scale) {
    if (begin >= end) return 0;
    
    bool negative = false;
    const char* ptr = begin;
    
    // Handle sign
    if (*ptr == '-') {
        negative = true;
        ++ptr;
    } else if (*ptr == '+') {
        ++ptr;
    }
    
    if (ptr >= end) return 0;
    
    int64_t integer_part = 0;
    int64_t fractional_part = 0;
    int64_t fractional_scale = 1;
    
    // Parse integer part
    while (ptr < end && is_digit(*ptr)) {
        integer_part = integer_part * 10 + digit_to_int(*ptr);
        ++ptr;
    }
    
    // Parse fractional part if decimal point exists
    if (ptr < end && *ptr == '.') {
        ++ptr;
        
        while (ptr < end && is_digit(*ptr) && fractional_scale < scale) {
            fractional_part = fractional_part * 10 + digit_to_int(*ptr);
            fractional_scale *= 10;
            ++ptr;
        }
    }
    
    // Scale the result
    int64_t result = integer_part * scale + fractional_part * (scale / fractional_scale);
    
    return negative ? -result : result;
}

inline int64_t FastNumberParser::fast_atof_fixed(std::string_view str, int64_t scale) {
    return fast_atof_fixed(str.data(), str.data() + str.size(), scale);
}

inline uint32_t FastNumberParser::fast_atou(const char* begin, const char* end) {
    if (begin >= end) return 0;
    
    uint32_t result = 0;
    const char* ptr = begin;
    constexpr uint32_t MAX_SAFE = UINT32_MAX / 10;
    
    while (ptr < end && is_digit(*ptr)) {
        // Overflow protection
        if (result > MAX_SAFE) {
            return UINT32_MAX;
        }
        
        result = result * 10 + digit_to_int(*ptr);
        ++ptr;
    }
    
    return result;
}

inline uint32_t FastNumberParser::fast_atou(std::string_view str) {
    return fast_atou(str.data(), str.data() + str.size());
}

} // namespace parser
} // namespace feedhandler