// Codeforces String Task (Problem 118A)
// Problem: Given a string, remove all vowels and add a dot before each consonant,
// then convert to lowercase.
//
// Example: "tour" -> ".t.r"
// Example: "Codeforces" -> ".c.d.f.r.c.s"
//
// Approach: Single pass with character classification
// Time: O(n), Space: O(n) for output

#include <string>
#include <cctype>
#include <iostream>

class Solution {
public:
    std::string processString(const std::string& s) {
        std::string result;
        result.reserve(s.length() * 2);  // Worst case: all consonants
        
        for (char c : s) {
            // Convert to lowercase
            char lower = std::tolower(c);
            
            // Check if vowel
            if (is_vowel(lower)) {
                continue;  // Skip vowels
            }
            
            // Add dot and consonant
            result += '.';
            result += lower;
        }
        
        return result;
    }
    
private:
    bool is_vowel(char c) const {
        return c == 'a' || c == 'e' || c == 'i' || 
               c == 'o' || c == 'u' || c == 'y';
    }
};

// Optimized version using lookup table
class SolutionOptimized {
public:
    SolutionOptimized() {
        // Initialize vowel lookup table
        std::fill(is_vowel_table, is_vowel_table + 256, false);
        is_vowel_table['a'] = is_vowel_table['A'] = true;
        is_vowel_table['e'] = is_vowel_table['E'] = true;
        is_vowel_table['i'] = is_vowel_table['I'] = true;
        is_vowel_table['o'] = is_vowel_table['O'] = true;
        is_vowel_table['u'] = is_vowel_table['U'] = true;
        is_vowel_table['y'] = is_vowel_table['Y'] = true;
    }
    
    std::string processString(const std::string& s) {
        std::string result;
        result.reserve(s.length() * 2);
        
        for (char c : s) {
            // Skip vowels using lookup table (O(1))
            if (is_vowel_table[static_cast<unsigned char>(c)]) {
                continue;
            }
            
            // Add dot and lowercase consonant
            result += '.';
            result += std::tolower(c);
        }
        
        return result;
    }
    
private:
    bool is_vowel_table[256];
};

// Test cases:
// Input: "tour"
// Output: ".t.r"
//
// Input: "Codeforces"
// Output: ".c.d.f.r.c.s"
//
// Input: "aBAcAba"
// Output: ".b.c.b"
//
// Input: "aeiouy"
// Output: "" (all vowels removed)
//
// Input: "bcdfg"
// Output: ".b.c.d.f.g"

int main() {
    Solution sol;
    
    // Test cases
    std::cout << "Test 1: " << sol.processString("tour") << std::endl;
    // Expected: .t.r
    
    std::cout << "Test 2: " << sol.processString("Codeforces") << std::endl;
    // Expected: .c.d.f.r.c.s
    
    std::cout << "Test 3: " << sol.processString("aBAcAba") << std::endl;
    // Expected: .b.c.b
    
    std::cout << "Test 4: " << sol.processString("aeiouy") << std::endl;
    // Expected: (empty)
    
    std::cout << "Test 5: " << sol.processString("bcdfg") << std::endl;
    // Expected: .b.c.d.f.g
    
    return 0;
}

// Performance notes:
// - Lookup table approach is faster than multiple comparisons
// - Reserve capacity to avoid reallocations
// - Single pass through string
// - No temporary string allocations
//
// Relevance to FeedHandler:
// - Character classification (similar to delimiter detection)
// - Single-pass string processing
// - Lookup table optimization (like tag switch optimization)
// - Memory efficiency with reserve()
