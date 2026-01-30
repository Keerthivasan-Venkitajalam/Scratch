// LeetCode 151: Reverse Words in a String
// Problem: Given an input string s, reverse the order of the words.
// A word is defined as a sequence of non-space characters.
// The string may contain leading or trailing spaces or multiple spaces between words.
// You must reverse the order but still maintain internal word structure.
// Must do this in-place (no split() allowed) with O(1) extra memory.

#include <string>
#include <algorithm>

class Solution {
public:
    std::string reverseWords(std::string s) {
        // TODO: Implement in-place word reversal (no split allowed)
        reverse(s, 0, s.length() - 1);
        int start = 0;
        for (int i = 0; i <= s.length(); ++i) {
            if (i == s.length() || s[i] == ' ') {
                reverse(s, start, i - 1);
                start = i + 1;
            }
        }
        // Remove extra spaces
        std::string result;
        bool in_word = false;
        for (char c : s) {
            if (c != ' ') {
                if (in_word) {
                    result += c;  // Continue current word
                } else {
                    if (!result.empty()) {
                        result += ' ';  // Add space before new word
                    }
                    result += c;
                    in_word = true;
                }
            } else {
                in_word = false;
            }
        }
        return result;
    }
    
private:
    // Helper: reverse a portion of the string
    void reverse(std::string& s, int start, int end) {
        // TODO: Implement
        while (start < end) {
            std::swap(s[start], s[end]);
            start++;
            end--;
        }
    }
};

// Test cases:
// Input: s = "the sky is blue"
// Output: "blue is sky the"
//
// Input: s = "  hello world  "
// Output: "world hello"
//
// Input: s = "a good   example"
// Output: "example good a"
