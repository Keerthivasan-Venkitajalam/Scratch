#include "parser/simd_fix_parser.hpp"
#include <chrono>
#include <iostream>
#include <cstring>
#include <algorithm>

#ifdef __ARM_NEON
#include <arm_neon.h>
#else
#include <immintrin.h>
#endif

namespace feedhandler {
namespace parser {

SIMDFixParser::SIMDFixParser() : buffer_pos_(0) {
    std::memset(processing_buffer_, 0, sizeof(processing_buffer_));
}

void SIMDFixParser::reset() {
    buffer_pos_ = 0;
}

std::vector<size_t> SIMDFixParser::find_delimiters_simd(const char* data, size_t length, char delimiter) {
    std::vector<size_t> positions;
    positions.reserve(length / 10); // Estimate
    
#ifdef __ARM_NEON
    // ARM NEON implementation
    uint8x16_t delimiter_vec = vdupq_n_u8(delimiter);
    
    size_t i = 0;
    // Process 16 bytes at a time with NEON
    for (; i + 15 < length; i += 16) {
        // Load 16 bytes from input
        uint8x16_t data_vec = vld1q_u8(reinterpret_cast<const uint8_t*>(data + i));
        
        // Compare with delimiter
        uint8x16_t cmp_result = vceqq_u8(data_vec, delimiter_vec);
        
        // Extract matches
        uint64_t mask_low = vgetq_lane_u64(vreinterpretq_u64_u8(cmp_result), 0);
        uint64_t mask_high = vgetq_lane_u64(vreinterpretq_u64_u8(cmp_result), 1);
        
        // Process low 8 bytes
        for (int bit = 0; bit < 64; bit += 8) {
            if ((mask_low >> bit) & 0xFF) {
                positions.push_back(i + bit / 8);
            }
        }
        
        // Process high 8 bytes
        for (int bit = 0; bit < 64; bit += 8) {
            if ((mask_high >> bit) & 0xFF) {
                positions.push_back(i + 8 + bit / 8);
            }
        }
    }
#else
    // x86 AVX2 implementation
    __m256i delimiter_vec = _mm256_set1_epi8(delimiter);
    
    size_t i = 0;
    // Process 32 bytes at a time with AVX2
    for (; i + 31 < length; i += 32) {
        // Load 32 bytes from input
        __m256i data_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i));
        
        // Compare with delimiter
        __m256i cmp_result = _mm256_cmpeq_epi8(data_vec, delimiter_vec);
        
        // Get bitmask of matches
        uint32_t mask = _mm256_movemask_epi8(cmp_result);
        
        // Extract positions of matches
        while (mask != 0) {
            int pos = __builtin_ctz(mask); // Count trailing zeros
            positions.push_back(i + pos);
            mask &= mask - 1; // Clear lowest set bit
        }
    }
#endif
    
    // Handle remaining bytes
    for (; i < length; ++i) {
        if (data[i] == delimiter) {
            positions.push_back(i);
        }
    }
    
    return positions;
}
int SIMDFixParser::simd_atoi(std::string_view str) {
    if (str.empty()) return 0;
    
    const char* data = str.data();
    size_t len = str.size();
    
    // Handle sign
    bool negative = false;
    size_t start = 0;
    if (data[0] == '-') {
        negative = true;
        start = 1;
    } else if (data[0] == '+') {
        start = 1;
    }
    
    int result = 0;
    
    // SIMD optimization for long numbers (8+ digits)
    if (len - start >= 8) {
#ifdef __ARM_NEON
        // ARM NEON implementation
        uint8x8_t digits = vld1_u8(reinterpret_cast<const uint8_t*>(data + start));
        uint8x8_t zero = vdup_n_u8('0');
        uint8x8_t converted = vsub_u8(digits, zero);
        
        // Extract and combine
        uint8_t extracted[8];
        vst1_u8(extracted, converted);
        
        // Combine digits with powers of 10
        for (size_t i = 0; i < std::min(len - start, size_t(8)); ++i) {
            if (extracted[i] <= 9) {
                result = result * 10 + extracted[i];
            }
        }
        start += 8;
#else
        // x86 SSE implementation
        __m128i digits = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + start));
        __m128i zero = _mm_set1_epi8('0');
        __m128i converted = _mm_sub_epi8(digits, zero);
        
        // Extract and combine
        alignas(16) char extracted[16];
        _mm_store_si128(reinterpret_cast<__m128i*>(extracted), converted);
        
        // Combine digits with powers of 10
        for (size_t i = 0; i < std::min(len - start, size_t(8)); ++i) {
            if (extracted[i] >= 0 && extracted[i] <= 9) {
                result = result * 10 + extracted[i];
            }
        }
        start += 8;
#endif
    }
    
    // Handle remaining digits
    for (size_t i = start; i < len; ++i) {
        if (data[i] >= '0' && data[i] <= '9') {
            result = result * 10 + (data[i] - '0');
        } else {
            break;
        }
    }
    
    return negative ? -result : result;
}

size_t SIMDFixParser::parse(const char* data, size_t length, std::vector<common::Tick>& ticks) {
    if (!data || length == 0) return 0;
    
    // Copy data to processing buffer for SIMD alignment
    size_t copy_size = std::min(length, sizeof(processing_buffer_) - buffer_pos_);
    std::memcpy(processing_buffer_ + buffer_pos_, data, copy_size);
    buffer_pos_ += copy_size;
    
    // Find all SOH delimiters using SIMD
    auto delimiters = find_delimiters_simd(processing_buffer_, buffer_pos_, '\x01');
    
    size_t processed = 0;
    size_t start = 0;
    
    for (size_t delimiter_pos : delimiters) {
        if (delimiter_pos <= start) continue;
        
        // Extract field using zero-copy string_view
        std::string_view field(processing_buffer_ + start, delimiter_pos - start);
        
        // Find tag separator
        size_t eq_pos = field.find('=');
        if (eq_pos == std::string_view::npos) {
            start = delimiter_pos + 1;
            continue;
        }
        
        // Parse tag and value using SIMD
        int tag = simd_atoi(field.substr(0, eq_pos));
        std::string_view value = field.substr(eq_pos + 1);
        
        // Process critical tags for tick construction
        static common::Tick current_tick;
        
        switch (tag) {
            case 55: // Symbol
                current_tick.symbol = std::string(value);
                break;
            case 44: // Price
                current_tick.price = simd_atof_fixed(value);
                break;
            case 38: // Quantity
                current_tick.qty = simd_atoi(value);
                break;
            case 52: // SendingTime
                current_tick.timestamp = parse_timestamp_simd(value);
                break;
            case 10: // Checksum - end of message
                ticks.push_back(current_tick);
                current_tick = {}; // Reset
                break;
        }
        
        start = delimiter_pos + 1;
        processed = start;
    }
    
    // Move unprocessed data to beginning of buffer
    if (processed < buffer_pos_) {
        std::memmove(processing_buffer_, processing_buffer_ + processed, buffer_pos_ - processed);
        buffer_pos_ -= processed;
    } else {
        buffer_pos_ = 0;
    }
    
    return std::min(processed, copy_size);
}

double SIMDFixParser::simd_atof(std::string_view str) {
    if (str.empty()) return 0.0;
    
    // Fast path for common price formats like "150.25"
    const char* data = str.data();
    size_t len = str.size();
    
    double result = 0.0;
    bool negative = false;
    size_t i = 0;
    
    if (data[0] == '-') {
        negative = true;
        i = 1;
    }
    
    // Parse integer part
    for (; i < len && data[i] != '.'; ++i) {
        if (data[i] >= '0' && data[i] <= '9') {
            result = result * 10.0 + (data[i] - '0');
        }
    }
    
    // Parse fractional part
    if (i < len && data[i] == '.') {
        ++i;
        double fraction = 0.0;
        double divisor = 10.0;
        
        for (; i < len; ++i) {
            if (data[i] >= '0' && data[i] <= '9') {
                fraction += (data[i] - '0') / divisor;
                divisor *= 10.0;
            }
        }
        result += fraction;
    }
    
    return negative ? -result : result;
}

uint64_t SIMDFixParser::parse_timestamp_simd(std::string_view timestamp_str) {
    // Fast timestamp parsing for FIX format: YYYYMMDD-HH:MM:SS
    if (timestamp_str.size() < 17) return 0;
    
    // For now, just return current timestamp (simplified implementation)
    // In production, would parse the actual timestamp components
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

int64_t SIMDFixParser::simd_atof_fixed(std::string_view str) {
    // Convert to fixed-point representation (multiply by 10000 for 4 decimal places)
    double value = simd_atof(str);
    return static_cast<int64_t>(value * 10000.0);
}

uint64_t SIMDFixParser::benchmark_parsing(size_t message_count) {
    SIMDFixParser parser;
    std::vector<common::Tick> ticks;
    ticks.reserve(message_count);
    
    std::string test_message = 
        "8=FIX.4.4\x01" "9=79\x01" "35=D\x01" "55=AAPL\x01" 
        "44=150.2500\x01" "38=500\x01" "54=1\x01" 
        "52=20240131-12:34:56\x01" "10=020\x01";
    
    std::string buffer;
    buffer.reserve(test_message.size() * message_count);
    for (size_t i = 0; i < message_count; ++i) {
        buffer += test_message;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    parser.parse(buffer.data(), buffer.size(), ticks);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    return duration.count();
}

} // namespace parser
} // namespace feedhandler