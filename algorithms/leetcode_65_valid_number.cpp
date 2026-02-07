/*
 * LeetCode 65: Valid Number
 * 
 * Given a string s, return whether s is a valid number.
 * 
 * A valid number can be split into these components (in order):
 * 1. A decimal number or an integer
 * 2. (Optional) An 'e' or 'E', followed by an integer
 * 
 * A decimal number can be split into these components (in order):
 * 1. (Optional) A sign character ('+' or '-')
 * 2. One of the following formats:
 *    - One or more digits, followed by a dot '.'
 *    - One or more digits, followed by a dot '.', followed by one or more digits
 *    - A dot '.', followed by one or more digits
 * 
 * An integer can be split into these components (in order):
 * 1. (Optional) A sign character ('+' or '-')
 * 2. One or more digits
 * 
 * Difficulty: Hard
 * Topics: String, Finite State Machine
 */

#include <string>
#include <iostream>
#include <cassert>

class Solution {
public:
    enum State {
        STATE_INITIAL,          // 0: Initial state
        STATE_SIGN,             // 1: Sign seen
        STATE_INTEGER,          // 2: Integer part
        STATE_POINT,            // 3: Decimal point seen
        STATE_FRACTION,         // 4: Fraction part
        STATE_EXP,              // 5: Exponent 'e' or 'E' seen
        STATE_EXP_SIGN,         // 6: Exponent sign seen
        STATE_EXP_NUMBER,       // 7: Exponent number
        STATE_END               // 8: End state (valid)
    };
    
    bool isNumber(std::string s) {
        State state = STATE_INITIAL;
        
        for (char c : s) {
            switch (state) {
                case STATE_INITIAL:
                    if (c == '+' || c == '-') {
                        state = STATE_SIGN;
                    } else if (isdigit(c)) {
                        state = STATE_INTEGER;
                    } else if (c == '.') {
                        state = STATE_POINT;
                    } else {
                        return false;
                    }
                    break;
                    
                case STATE_SIGN:
                    if (isdigit(c)) {
                        state = STATE_INTEGER;
                    } else if (c == '.') {
                        state = STATE_POINT;
                    } else {
                        return false;
                    }
                    break;
                    
                case STATE_INTEGER:
                    if (isdigit(c)) {
                        // Stay in STATE_INTEGER
                    } else if (c == '.') {
                        state = STATE_FRACTION;
                    } else if (c == 'e' || c == 'E') {
                        state = STATE_EXP;
                    } else {
                        return false;
                    }
                    break;
                    
                case STATE_POINT:
                    if (isdigit(c)) {
                        state = STATE_FRACTION;
                    } else {
                        return false;
                    }
                    break;
                    
                case STATE_FRACTION:
                    if (isdigit(c)) {
                        // Stay in STATE_FRACTION
                    } else if (c == 'e' || c == 'E') {
                        state = STATE_EXP;
                    } else {
                        return false;
                    }
                    break;
                    
                case STATE_EXP:
                    if (c == '+' || c == '-') {
                        state = STATE_EXP_SIGN;
                    } else if (isdigit(c)) {
                        state = STATE_EXP_NUMBER;
                    } else {
                        return false;
                    }
                    break;
                    
                case STATE_EXP_SIGN:
                    if (isdigit(c)) {
                        state = STATE_EXP_NUMBER;
                    } else {
                        return false;
                    }
                    break;
                    
                case STATE_EXP_NUMBER:
                    if (isdigit(c)) {
                        // Stay in STATE_EXP_NUMBER
                    } else {
                        return false;
                    }
                    break;
                    
                default:
                    return false;
            }
        }
        
        // Valid end states
        return state == STATE_INTEGER || 
               state == STATE_FRACTION || 
               state == STATE_EXP_NUMBER;
    }
};

// Test cases
void test_valid_number() {
    Solution sol;
    
    std::cout << "Valid Numbers:" << std::endl;
    
    // Valid integers
    assert(sol.isNumber("2") == true);
    std::cout << "✓ \"2\" -> true" << std::endl;
    
    assert(sol.isNumber("0089") == true);
    std::cout << "✓ \"0089\" -> true" << std::endl;
    
    assert(sol.isNumber("-0.1") == true);
    std::cout << "✓ \"-0.1\" -> true" << std::endl;
    
    assert(sol.isNumber("+3.14") == true);
    std::cout << "✓ \"+3.14\" -> true" << std::endl;
    
    // Valid decimals
    assert(sol.isNumber("4.") == true);
    std::cout << "✓ \"4.\" -> true" << std::endl;
    
    assert(sol.isNumber("-.9") == true);
    std::cout << "✓ \"-.9\" -> true" << std::endl;
    
    // Valid scientific notation
    assert(sol.isNumber("2e10") == true);
    std::cout << "✓ \"2e10\" -> true" << std::endl;
    
    assert(sol.isNumber("-90E3") == true);
    std::cout << "✓ \"-90E3\" -> true" << std::endl;
    
    assert(sol.isNumber("3e+7") == true);
    std::cout << "✓ \"3e+7\" -> true" << std::endl;
    
    assert(sol.isNumber("+6e-1") == true);
    std::cout << "✓ \"+6e-1\" -> true" << std::endl;
    
    assert(sol.isNumber("53.5e93") == true);
    std::cout << "✓ \"53.5e93\" -> true" << std::endl;
    
    assert(sol.isNumber("-123.456e789") == true);
    std::cout << "✓ \"-123.456e789\" -> true" << std::endl;
    
    std::cout << "\nInvalid Numbers:" << std::endl;
    
    // Invalid cases
    assert(sol.isNumber("abc") == false);
    std::cout << "✓ \"abc\" -> false" << std::endl;
    
    assert(sol.isNumber("1a") == false);
    std::cout << "✓ \"1a\" -> false" << std::endl;
    
    assert(sol.isNumber("1e") == false);
    std::cout << "✓ \"1e\" -> false" << std::endl;
    
    assert(sol.isNumber("e3") == false);
    std::cout << "✓ \"e3\" -> false" << std::endl;
    
    assert(sol.isNumber("99e2.5") == false);
    std::cout << "✓ \"99e2.5\" -> false" << std::endl;
    
    assert(sol.isNumber("--6") == false);
    std::cout << "✓ \"--6\" -> false" << std::endl;
    
    assert(sol.isNumber("-+3") == false);
    std::cout << "✓ \"-+3\" -> false" << std::endl;
    
    assert(sol.isNumber("95a54e53") == false);
    std::cout << "✓ \"95a54e53\" -> false" << std::endl;
    
    assert(sol.isNumber(".") == false);
    std::cout << "✓ \".\" -> false" << std::endl;
    
    assert(sol.isNumber(".e1") == false);
    std::cout << "✓ \".e1\" -> false" << std::endl;
    
    std::cout << "\n All valid number tests passed!" << std::endl;
}

int main() {
    std::cout << "LeetCode 65: Valid Number (FSM)" << std::endl;
    std::cout << "================================" << std::endl;
    std::cout << std::endl;
    
    test_valid_number();
    
    return 0;
}