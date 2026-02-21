// Day 6 Algorithm Sprint Solutions
// 
// Store completed LeetCode solutions here after submission.
// Include:
//  - Problem number and name
//  - Time/space complexity
//  - Brief approach explanation
//  - Acceptance date

// ============================================
// LeetCode 344: Reverse String
// Status: COMPLETED ✓
// Time: O(n)  Space: O(1)
// Approach: Two-pointer swap from ends toward center
// Date: Week 1 Day 6
// ============================================

// ============================================
// LeetCode 151: Reverse Words in a String
// Status: COMPLETED ✓
// Time: O(n)  Space: O(n) [output string]
// Approach: 
//   1. Reverse entire string
//   2. Reverse each word individually
//   3. Clean up extra spaces with state tracking
// Date: Week 1 Day 6
// ============================================

// ============================================
// LeetCode 8: String to Integer (atoi)
// Status: COMPLETED ✓
// Time: O(n)  Space: O(1)
// Approach:
//   1. Skip leading whitespace
//   2. Check for optional '+' or '-' sign
//   3. Read digits until non-digit character
//   4. Clamp result to [INT_MIN, INT_MAX]
// Date: Week 2 Day 6
// ============================================

// ============================================
// LeetCode 65: Valid Number (FSM)
// Status: COMPLETED ✓
// Time: O(n)  Space: O(1)
// Approach:
//   - Finite State Machine with 8 states
//   - States: INITIAL, SIGN, INTEGER, POINT, FRACTION, EXP, EXP_SIGN, EXP_NUMBER
//   - Valid end states: INTEGER, FRACTION, EXP_NUMBER
//   - Handles integers, decimals, scientific notation
// Date: Week 2 Day 6
// ============================================

// ============================================
// LeetCode 10: Regular Expression Matching
// Status: COMPLETED ✓
// Time: O(m*n)  Space: O(m*n)
// Approach:
//   - Dynamic Programming solution
//   - dp[i][j] = true if s[0..i-1] matches p[0..j-1]
//   - Handle '.' (matches any single char) and '*' (matches 0+ of preceding)
//   - Star can match zero occurrences or one+ if preceding element matches
// Date: Week 3 Day 6
// ============================================

// ============================================
// LeetCode 151: Reverse Words (OPTIMIZED)
// Status: COMPLETED ✓
// Time: O(n)  Space: O(1)
// Approach:
//   1. Remove extra spaces in-place using two-pointer technique
//   2. Reverse entire string
//   3. Reverse each word individually
//   - TRUE in-place with no extra string allocation
// Date: Week 3 Day 6
// ============================================

// ============================================
// LeetCode 3: Longest Substring Without Repeating Characters
// Status: COMPLETED ✓
// Time: O(n)  Space: O(min(n, m)) where m is charset size
// Approach:
//   - Sliding window with hash set
//   - Expand right pointer, contract left when duplicate found
//   - Optimized version uses array to track last seen position
//   - Jump start pointer to avoid removing elements one by one
// Date: Week 4 Day 6
// ============================================

// ============================================
// Codeforces 118A: String Task
// Status: COMPLETED ✓
// Time: O(n)  Space: O(n)
// Approach:
//   - Remove vowels (a, e, i, o, u, y)
//   - Add dot before each consonant
//   - Convert to lowercase
//   - Optimized with lookup table for O(1) vowel check
// Date: Week 4 Day 6
// ============================================