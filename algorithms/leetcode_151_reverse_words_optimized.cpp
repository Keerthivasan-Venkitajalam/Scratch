// LeetCode 151: Reverse Words in a String (OPTIMIZED IN-PLACE)
// Problem: Given an input string s, reverse the order of the words.
// This version does TRUE in-place modification with O(1) extra space.
// No extra string allocation - modifies the input string directly.

#include <string>
#include <algorithm>

class Solution {
public:
    std::string reverseWords(std::string s) {
        // Step 1: Remove leading, trailing, and extra spaces in-place
        int write_idx = 0;
        bool in_word = false;
        
        for (int i = 0; i < s.length(); i++) {
            if (s[i] != ' ') {
                // Add space before new word (except for first word)
                if (write_idx != 0 && !in_word) {
                    s[write_idx++] = ' ';
                }
                s[write_idx++] = s[i];
                in_word = true;
            } else {
                in_word = false;
            }
        }
        
        // Resize to actual content length
        s.resize(write_idx);
        
        // Step 2: Reverse the entire string
        reverse(s.begin(), s.end());
        
        // Step 3: Reverse each word individually
        int start = 0;
        for (int i = 0; i <= s.length(); i++) {
            if (i == s.length() || s[i] == ' ') {
                reverse(s.begin() + start, s.begin() + i);
                start = i + 1;
            }
        }
        
        return s;
    }
};

// Algorithm explanation:
// 1. Clean up spaces in-place by using two-pointer technique
// 2. Reverse entire string: "the sky is blue" -> "eulb si yks eht"
// 3. Reverse each word: "eulb si yks eht" -> "blue is sky the"
//
// Time: O(n)
// Space: O(1) - only modifies input string, no extra allocation
//
// Test cases:
// Input: s = "the sky is blue"
// Output: "blue is sky the"
//
// Input: s = "  hello world  "
// Output: "world hello"
//
// Input: s = "a good   example"
// Output: "example good a"

