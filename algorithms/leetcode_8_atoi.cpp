/*
 * LeetCode 8: String to Integer (atoi)
 * 
 * Implement the myAtoi(string s) function, which converts a string to a 32-bit signed integer.
 * 
 * Algorithm:
 * 1. Whitespace: Ignore leading whitespace
 * 2. Signedness: Check for '+' or '-' sign
 * 3. Conversion: Read digits until non-digit or end of string
 * 4. Rounding: Clamp to [INT_MIN, INT_MAX] range
 * 
 * Difficulty: Medium
 * Topics: String, Implementation
 */

#include <string>
#include <climits>
#include <iostream>
#include <cassert>

class Solution {
public:
    int myAtoi(std::string s) {
        int i = 0;
        int n = s.length();
        
        // Step 1: Skip leading whitespace
        while (i < n && s[i] == ' ') {
            ++i;
        }
        
        // Step 2: Check for sign
        int sign = 1;
        if (i < n && (s[i] == '+' || s[i] == '-')) {
            sign = (s[i] == '-') ? -1 : 1;
            ++i;
        }
        
        // Step 3: Convert digits
        long result = 0;  // Use long to detect overflow
        
        while (i < n && isdigit(s[i])) {
            int digit = s[i] - '0';
            result = result * 10 + digit;
            
            // Step 4: Check for overflow/underflow
            if (sign == 1 && result > INT_MAX) {
                return INT_MAX;
            }
            if (sign == -1 && -result < INT_MIN) {
                return INT_MIN;
            }
            
            ++i;
        }
        
        return static_cast<int>(sign * result);
    }
};

// Test cases
void test_atoi() {
    Solution sol;
    
    // Test case 1: Basic positive number
    assert(sol.myAtoi("42") == 42);
    std::cout << "✓ Test 1 passed: \"42\" -> 42" << std::endl;
    
    // Test case 2: Leading whitespace
    assert(sol.myAtoi("   -42") == -42);
    std::cout << "✓ Test 2 passed: \"   -42\" -> -42" << std::endl;
    
    // Test case 3: Words after digits
    assert(sol.myAtoi("4193 with words") == 4193);
    std::cout << "✓ Test 3 passed: \"4193 with words\" -> 4193" << std::endl;
    
    // Test case 4: Overflow
    assert(sol.myAtoi("91283472332") == INT_MAX);
    std::cout << "✓ Test 4 passed: \"91283472332\" -> INT_MAX" << std::endl;
    
    // Test case 5: Underflow
    assert(sol.myAtoi("-91283472332") == INT_MIN);
    std::cout << "✓ Test 5 passed: \"-91283472332\" -> INT_MIN" << std::endl;
    
    // Test case 6: No digits
    assert(sol.myAtoi("words and 987") == 0);
    std::cout << "✓ Test 6 passed: \"words and 987\" -> 0" << std::endl;
    
    // Test case 7: Zero
    assert(sol.myAtoi("0") == 0);
    std::cout << "✓ Test 7 passed: \"0\" -> 0" << std::endl;
    
    // Test case 8: Positive sign
    assert(sol.myAtoi("+123") == 123);
    std::cout << "✓ Test 8 passed: \"+123\" -> 123" << std::endl;
    
    // Test case 9: Only whitespace
    assert(sol.myAtoi("   ") == 0);
    std::cout << "✓ Test 9 passed: \"   \" -> 0" << std::endl;
    
    // Test case 10: Leading zeros
    assert(sol.myAtoi("00042") == 42);
    std::cout << "✓ Test 10 passed: \"00042\" -> 42" << std::endl;
    
    std::cout << "\n All atoi tests passed!" << std::endl;
}

int main() {
    std::cout << "LeetCode 8: String to Integer (atoi)" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << std::endl;
    
    test_atoi();
    
    return 0;
}