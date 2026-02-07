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