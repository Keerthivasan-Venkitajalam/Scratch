// LeetCode 295: Find Median from Data Stream
// Problem: Design a data structure that supports:
// - addNum(num): Add integer to data structure
// - findMedian(): Return median of all elements so far
//
// Solution: Two heaps (max heap + min heap)
// - Max heap: stores smaller half of numbers
// - Min heap: stores larger half of numbers
// - Median is either top of max heap, or average of both tops

#include <queue>
#include <vector>
#include <functional>

class MedianFinder {
private:
    // Max heap for smaller half (largest element at top)
    std::priority_queue<int> max_heap_;
    
    // Min heap for larger half (smallest element at top)
    std::priority_queue<int, std::vector<int>, std::greater<int>> min_heap_;
    
public:
    MedianFinder() {}
    
    void addNum(int num) {
        // Always add to max heap first
        max_heap_.push(num);
        
        // Balance: move largest from max heap to min heap
        min_heap_.push(max_heap_.top());
        max_heap_.pop();
        
        // Maintain size property: max_heap.size() >= min_heap.size()
        if (max_heap_.size() < min_heap_.size()) {
            max_heap_.push(min_heap_.top());
            min_heap_.pop();
        }
    }
    
    double findMedian() {
        if (max_heap_.size() > min_heap_.size()) {
            // Odd number of elements - median is top of max heap
            return max_heap_.top();
        } else {
            // Even number of elements - median is average of both tops
            return (max_heap_.top() + min_heap_.top()) / 2.0;
        }
    }
};

/**
 * Time Complexity:
 * - addNum(): O(log n) - heap insertion
 * - findMedian(): O(1) - just peek at heap tops
 * 
 * Space Complexity: O(n) - store all numbers
 * 
 * Key Insights:
 * 1. Max heap stores smaller half, min heap stores larger half
 * 2. Max heap size = min heap size OR max heap size = min heap size + 1
 * 3. Median is always accessible at heap tops
 * 4. Balancing ensures O(1) median query
 * 
 * Invariants:
 * - max_heap.top() <= min_heap.top() (if both non-empty)
 * - |max_heap.size() - min_heap.size()| <= 1
 * - max_heap.size() >= min_heap.size()
 * 
 * Example:
 * addNum(1)    -> max_heap: [1], min_heap: []        -> median: 1
 * addNum(2)    -> max_heap: [1], min_heap: [2]       -> median: 1.5
 * findMedian() -> returns 1.5
 * addNum(3)    -> max_heap: [2,1], min_heap: [3]     -> median: 2
 * findMedian() -> returns 2
 * 
 * Why this works:
 * - Smaller half in max heap: easy access to largest of smaller numbers
 * - Larger half in min heap: easy access to smallest of larger numbers
 * - Median is either the largest small number, or average of boundary numbers
 */

// Test cases
#include <cassert>
#include <iostream>
#include <cmath>

void test_median_finder() {
    std::cout << "Testing Median Finder..." << std::endl;
    
    MedianFinder mf;
    
    mf.addNum(1);
    assert(std::abs(mf.findMedian() - 1.0) < 0.001);
    
    mf.addNum(2);
    assert(std::abs(mf.findMedian() - 1.5) < 0.001);
    
    mf.addNum(3);
    assert(std::abs(mf.findMedian() - 2.0) < 0.001);
    
    std::cout << "All Median Finder tests passed!" << std::endl;
}

void test_median_finder_extended() {
    std::cout << "Testing Median Finder (Extended)..." << std::endl;
    
    MedianFinder mf;
    
    // Add numbers: 5, 15, 1, 3
    mf.addNum(5);
    assert(std::abs(mf.findMedian() - 5.0) < 0.001);  // [5] -> 5
    
    mf.addNum(15);
    assert(std::abs(mf.findMedian() - 10.0) < 0.001); // [5, 15] -> 10
    
    mf.addNum(1);
    assert(std::abs(mf.findMedian() - 5.0) < 0.001);  // [1, 5, 15] -> 5
    
    mf.addNum(3);
    assert(std::abs(mf.findMedian() - 4.0) < 0.001);  // [1, 3, 5, 15] -> 4
    
    std::cout << "All Extended Median Finder tests passed!" << std::endl;
}

int main() {
    test_median_finder();
    test_median_finder_extended();
    
    std::cout << "\nMedian Finder Implementation:" << std::endl;
    std::cout << "- Uses two heaps (max heap + min heap)" << std::endl;
    std::cout << "- addNum(): O(log n)" << std::endl;
    std::cout << "- findMedian(): O(1)" << std::endl;
    std::cout << "- Perfect for streaming data!" << std::endl;
    
    return 0;
}
