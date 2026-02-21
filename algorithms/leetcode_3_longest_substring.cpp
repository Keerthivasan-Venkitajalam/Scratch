// LeetCode 3: Longest Substring Without Repeating Characters
// Problem: Given a string s, find the length of the longest substring without repeating characters.
// 
// Approach: Sliding window with hash set
// Time: O(n), Space: O(min(n, m)) where m is charset size

#include <string>
#include <unordered_set>
#include <algorithm>

class Solution {
public:
    int lengthOfLongestSubstring(std::string s) {
        std::unordered_set<char> char_set;
        int max_length = 0;
        int left = 0;
        
        // Sliding window: expand right, contract left when duplicate found
        for (int right = 0; right < s.length(); ++right) {
            // If character already in set, remove from left until it's gone
            while (char_set.count(s[right])) {
                char_set.erase(s[left]);
                left++;
            }
            
            // Add current character
            char_set.insert(s[right]);
            
            // Update max length
            max_length = std::max(max_length, right - left + 1);
        }
        
        return max_length;
    }
    
    // Alternative: Using array for ASCII characters (faster)
    int lengthOfLongestSubstringOptimized(std::string s) {
        // Track last seen position of each character
        int last_seen[256];
        std::fill(last_seen, last_seen + 256, -1);
        
        int max_length = 0;
        int start = 0;
        
        for (int i = 0; i < s.length(); ++i) {
            char c = s[i];
            
            // If character was seen after current start, move start
            if (last_seen[c] >= start) {
                start = last_seen[c] + 1;
            }
            
            // Update last seen position
            last_seen[c] = i;
            
            // Update max length
            max_length = std::max(max_length, i - start + 1);
        }
        
        return max_length;
    }
};

// Test cases:
// Input: s = "abcabcbb"
// Output: 3 (substring "abc")
//
// Input: s = "bbbbb"
// Output: 1 (substring "b")
//
// Input: s = "pwwkew"
// Output: 3 (substring "wke")
//
// Input: s = ""
// Output: 0
//
// Input: s = "dvdf"
// Output: 3 (substring "vdf")

// Explanation of optimized approach:
// Instead of using a set and removing elements, we track the last position
// where each character was seen. When we encounter a duplicate, we jump
// the start pointer to just after the previous occurrence.
//
// Example: "abcabcbb"
// i=0: a, start=0, len=1, last_seen[a]=0
// i=1: b, start=0, len=2, last_seen[b]=1
// i=2: c, start=0, len=3, last_seen[c]=2
// i=3: a, start=1 (jump past previous 'a'), len=3, last_seen[a]=3
// i=4: b, start=2 (jump past previous 'b'), len=3, last_seen[b]=4
// i=5: c, start=3 (jump past previous 'c'), len=3, last_seen[c]=5
// i=6: b, start=5 (jump past previous 'b'), len=2, last_seen[b]=6
// i=7: b, start=7 (jump past previous 'b'), len=1, last_seen[b]=7
// Result: 3
