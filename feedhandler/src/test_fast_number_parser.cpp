#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <chrono>
#include "parser/fast_number_parser.hpp"

using namespace feedhandler::parser;

// Test result tracking
struct TestResult {
    std::string test_name;
    bool passed;
    std::string error_message;
};

std::vector<TestResult> test_results;

void assert_test(const std::string& test_name, bool condition, const std::string& error_msg = "") {
    test_results.push_back({test_name, condition, error_msg});
    if (!condition) {
        std::cout << "FAIL: " << test_name << " - " << error_msg << std::endl;
    }
}

void test_fast_atoi_basic() {
    std::cout << "=== Testing fast_atoi Basic Cases ===" << std::endl;
    
    // Positive numbers
    assert_test("atoi_positive_123", FastNumberParser::fast_atoi("123") == 123);
    assert_test("atoi_positive_0", FastNumberParser::fast_atoi("0") == 0);
    assert_test("atoi_positive_single", FastNumberParser::fast_atoi("7") == 7);
    
    // Negative numbers
    assert_test("atoi_negative_123", FastNumberParser::fast_atoi("-123") == -123);
    assert_test("atoi_negative_single", FastNumberParser::fast_atoi("-7") == -7);
    
    // With plus sign
    assert_test("atoi_plus_sign", FastNumberParser::fast_atoi("+456") == 456);
    
    // Edge cases
    assert_test("atoi_empty", FastNumberParser::fast_atoi("") == 0);
    assert_test("atoi_just_sign", FastNumberParser::fast_atoi("-") == 0);
    assert_test("atoi_just_plus", FastNumberParser::fast_atoi("+") == 0);
}

void test_fast_atoi_invalid() {
    std::cout << "=== Testing fast_atoi Invalid Cases ===" << std::endl;
    
    // Non-numeric strings
    assert_test("atoi_letters", FastNumberParser::fast_atoi("abc") == 0);
    assert_test("atoi_mixed_start", FastNumberParser::fast_atoi("a123") == 0);
    
    // Numbers with trailing characters (should parse up to first non-digit)
    assert_test("atoi_trailing_letters", FastNumberParser::fast_atoi("123abc") == 123);
    assert_test("atoi_trailing_space", FastNumberParser::fast_atoi("456 ") == 456);
}

void test_fast_atoi_range() {
    std::cout << "=== Testing fast_atoi Range Cases ===" << std::endl;
    
    // Test with char* range
    const char* str = "12345xyz";
    assert_test("atoi_range_full", FastNumberParser::fast_atoi(str, str + 5) == 12345);
    assert_test("atoi_range_partial", FastNumberParser::fast_atoi(str, str + 3) == 123);
    assert_test("atoi_range_single", FastNumberParser::fast_atoi(str, str + 1) == 1);
    assert_test("atoi_range_empty", FastNumberParser::fast_atoi(str, str) == 0);
}

void test_fast_atof_fixed_basic() {
    std::cout << "=== Testing fast_atof_fixed Basic Cases ===" << std::endl;
    
    // Integer values (no decimal point)
    assert_test("atof_integer_123", FastNumberParser::fast_atof_fixed("123") == 1230000);
    assert_test("atof_integer_0", FastNumberParser::fast_atof_fixed("0") == 0);
    
    // Decimal values
    assert_test("atof_decimal_123_45", FastNumberParser::fast_atof_fixed("123.45") == 1234500);
    assert_test("atof_decimal_0_1234", FastNumberParser::fast_atof_fixed("0.1234") == 1234);
    assert_test("atof_decimal_full_precision", FastNumberParser::fast_atof_fixed("123.4567") == 1234567);
    
    // Leading zeros
    assert_test("atof_leading_zero", FastNumberParser::fast_atof_fixed("0123.45") == 1234500);
    assert_test("atof_fractional_leading_zero", FastNumberParser::fast_atof_fixed("123.0456") == 1230456);
    
    // Negative values
    assert_test("atof_negative_decimal", FastNumberParser::fast_atof_fixed("-123.45") == -1234500);
    assert_test("atof_negative_integer", FastNumberParser::fast_atof_fixed("-456") == -4560000);
}

void test_fast_atof_fixed_edge_cases() {
    std::cout << "=== Testing fast_atof_fixed Edge Cases ===" << std::endl;
    
    // No decimal point
    assert_test("atof_no_decimal", FastNumberParser::fast_atof_fixed("789") == 7890000);
    
    // Decimal point at end
    assert_test("atof_decimal_at_end", FastNumberParser::fast_atof_fixed("123.") == 1230000);
    
    // Only fractional part
    assert_test("atof_only_fractional", FastNumberParser::fast_atof_fixed(".5678") == 5678);
    
    // More precision than scale
    assert_test("atof_excess_precision", FastNumberParser::fast_atof_fixed("123.456789") == 1234567);
    
    // Different scales
    assert_test("atof_scale_100", FastNumberParser::fast_atof_fixed("123.45", 100) == 12345);
    assert_test("atof_scale_1000", FastNumberParser::fast_atof_fixed("123.456", 1000) == 123456);
}

void test_fast_atou_basic() {
    std::cout << "=== Testing fast_atou Basic Cases ===" << std::endl;
    
    // Positive numbers only
    assert_test("atou_positive_123", FastNumberParser::fast_atou("123") == 123u);
    assert_test("atou_positive_0", FastNumberParser::fast_atou("0") == 0u);
    assert_test("atou_large_number", FastNumberParser::fast_atou("4294967295") == 4294967295u);
    
    // Invalid cases
    assert_test("atou_negative", FastNumberParser::fast_atou("-123") == 0u);
    assert_test("atou_empty", FastNumberParser::fast_atou("") == 0u);
    assert_test("atou_letters", FastNumberParser::fast_atou("abc") == 0u);
}

void benchmark_parsing_performance() {
    std::cout << "=== Performance Benchmark ===" << std::endl;
    
    const size_t iterations = 1000000;
    
    // Test data
    std::vector<std::string> int_strings = {"123", "456789", "-987", "0", "2147483647"};
    std::vector<std::string> float_strings = {"123.45", "0.1234", "987.6543", "-456.789", "0.0001"};
    
    // Benchmark fast_atoi
    auto start = std::chrono::high_resolution_clock::now();
    volatile int sum = 0;  // volatile to prevent optimization
    for (size_t i = 0; i < iterations; ++i) {
        sum += FastNumberParser::fast_atoi(int_strings[i % int_strings.size()]);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double ns_per_call = (static_cast<double>(duration.count()) * 1000.0) / iterations;
    
    std::cout << "fast_atoi: " << iterations << " calls in " << duration.count() << " μs" << std::endl;
    std::cout << "  Average: " << ns_per_call << " ns per call" << std::endl;
    
    // Benchmark fast_atof_fixed
    start = std::chrono::high_resolution_clock::now();
    volatile int64_t sum_fixed = 0;
    for (size_t i = 0; i < iterations; ++i) {
        sum_fixed += FastNumberParser::fast_atof_fixed(float_strings[i % float_strings.size()]);
    }
    end = std::chrono::high_resolution_clock::now();
    
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    ns_per_call = (static_cast<double>(duration.count()) * 1000.0) / iterations;
    
    std::cout << "fast_atof_fixed: " << iterations << " calls in " << duration.count() << " μs" << std::endl;
    std::cout << "  Average: " << ns_per_call << " ns per call" << std::endl;
    
    // Prevent optimization
    std::cout << "Sum check: " << sum << ", " << sum_fixed << std::endl;
}

void print_test_summary() {
    std::cout << "\n=== Test Summary ===" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& result : test_results) {
        if (result.passed) {
            ++passed;
        } else {
            ++failed;
            std::cout << "FAILED: " << result.test_name;
            if (!result.error_message.empty()) {
                std::cout << " - " << result.error_message;
            }
            std::cout << std::endl;
        }
    }
    
    std::cout << "Total tests: " << test_results.size() << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    if (failed == 0) {
        std::cout << "🎉 All tests passed!" << std::endl;
    }
}

int main() {
    std::cout << "Fast Number Parser Test Suite" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << std::endl;
    
    test_fast_atoi_basic();
    test_fast_atoi_invalid();
    test_fast_atoi_range();
    test_fast_atof_fixed_basic();
    test_fast_atof_fixed_edge_cases();
    test_fast_atou_basic();
    
    benchmark_parsing_performance();
    
    print_test_summary();
    
    return test_results.empty() || std::all_of(test_results.begin(), test_results.end(), 
                                               [](const TestResult& r) { return r.passed; }) ? 0 : 1;
}