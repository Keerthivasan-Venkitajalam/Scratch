// LeetCode 239: Sliding Window Maximum
// Problem: Given an array and a sliding window of size k, find the maximum in each window position
// 
// Example:
// Input: nums = [1,3,-1,-3,5,3,6,7], k = 3
// Output: [3,3,5,5,6,7]
// 
// Approach: Use deque to maintain indices of potential maximums in decreasing order
// Time: O(n), Space: O(k)

#include <vector>
#include <deque>
#include <algorithm>

class Solution {
public:
    std::vector<int> maxSlidingWindow(std::vector<int>& nums, int k) {
        std::vector<int> result;
        std::deque<int> dq;  // Store indices, not values
        
        for (int i = 0; i < nums.size(); ++i) {
            // Remove indices outside current window
            while (!dq.empty() && dq.front() < i - k + 1) {
                dq.pop_front();
            }
            
            // Remove indices of smaller elements (they can't be max)
            while (!dq.empty() && nums[dq.back()] < nums[i]) {
                dq.pop_back();
            }
            
            // Add current index
            dq.push_back(i);
            
            // Add max to result (front of deque is always max)
            if (i >= k - 1) {
                result.push_back(nums[dq.front()]);
            }
        }
        
        return result;
    }
};

// Key insights:
// 1. Deque maintains indices in decreasing order of their values
// 2. Front of deque is always the maximum in current window
// 3. Remove elements outside window from front
// 4. Remove smaller elements from back (they can never be max)
// 
// Why this works:
// - If nums[i] > nums[j] and i > j, then nums[j] can never be max
//   in any future window that contains nums[i]
// - So we can safely remove nums[j] from consideration
//
// Application to trading:
// - Finding highest/lowest price in rolling time window
// - Detecting support/resistance levels
// - Calculating rolling max drawdown

