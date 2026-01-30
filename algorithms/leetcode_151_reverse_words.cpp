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
        return s;
    }
    
private:
    // Helper: reverse a portion of the string
    void reverse(std::string& s, int start, int end) {
        // TODO: Implement
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
