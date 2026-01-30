// LeetCode 344: Reverse String
// Problem: Write a function that reverses a string.
// The input string is given as an array of characters s.
// You must do this by modifying the input array in-place with O(1) extra memory.

#include <vector>
#include <string>

class Solution {
public:
    void reverseString(std::vector<char>& s) {
        int left = 0;
        int right = s.size() - 1;
        while (left < right) {
            std::swap(s[left], s[right]);
            left++;
            right--;
        }
    }
};


// Test cases:
// Input: s = ["h","e","l","l","o"]
// Output: ["o","l","l","e","h"]
//
// Input: s = ["H","a","n","n","a","h"]
// Output: ["h","a","n","n","a","H"]
