// LeetCode 10: Regular Expression Matching
// Problem: Given an input string s and a pattern p, implement regular expression matching
// with support for '.' and '*' where:
// - '.' Matches any single character
// - '*' Matches zero or more of the preceding element
// The matching should cover the entire input string (not partial).

#include <string>
#include <vector>

class Solution {
public:
    bool isMatch(std::string s, std::string p) {
        int m = s.length();
        int n = p.length();
        
        // dp[i][j] = true if s[0..i-1] matches p[0..j-1]
        std::vector<std::vector<bool>> dp(m + 1, std::vector<bool>(n + 1, false));
        
        // Empty string matches empty pattern
        dp[0][0] = true;
        
        // Handle patterns like a*, a*b*, a*b*c* that can match empty string
        for (int j = 2; j <= n; j++) {
            if (p[j - 1] == '*') {
                dp[0][j] = dp[0][j - 2];
            }
        }
        
        // Fill the DP table
        for (int i = 1; i <= m; i++) {
            for (int j = 1; j <= n; j++) {
                if (p[j - 1] == '*') {
                    // Star can match zero occurrences of preceding element
                    dp[i][j] = dp[i][j - 2];
                    
                    // Or star can match one or more if preceding element matches current char
                    if (p[j - 2] == s[i - 1] || p[j - 2] == '.') {
                        dp[i][j] = dp[i][j] || dp[i - 1][j];
                    }
                } else if (p[j - 1] == '.' || p[j - 1] == s[i - 1]) {
                    // Current characters match
                    dp[i][j] = dp[i - 1][j - 1];
                }
            }
        }
        
        return dp[m][n];
    }
};

// Test cases:
// Input: s = "aa", p = "a"
// Output: false (pattern doesn't match entire string)
//
// Input: s = "aa", p = "a*"
// Output: true ('*' means zero or more 'a')
//
// Input: s = "ab", p = ".*"
// Output: true ('.*' means zero or more of any character)
//
// Input: s = "aab", p = "c*a*b"
// Output: true (c* matches zero 'c', a* matches two 'a', b matches 'b')
//
// Input: s = "mississippi", p = "mis*is*p*."
// Output: false

