// LeetCode 146: LRU Cache
// Problem: Design a data structure that follows Least Recently Used (LRU) cache constraints
// Operations: get(key) and put(key, value) both in O(1) time
//
// Solution: Hash map + Doubly linked list
// - Hash map: O(1) lookup by key
// - Doubly linked list: O(1) move to front, O(1) remove from back
// - Most recently used at front, least recently used at back

#include <unordered_map>
#include <list>

class LRUCache {
private:
    int capacity_;
    
    // Doubly linked list to maintain LRU order
    // Front = most recently used, Back = least recently used
    std::list<std::pair<int, int>> cache_list_;  // {key, value}
    
    // Hash map: key -> iterator to list node
    std::unordered_map<int, std::list<std::pair<int, int>>::iterator> cache_map_;
    
public:
    LRUCache(int capacity) : capacity_(capacity) {}
    
    int get(int key) {
        auto it = cache_map_.find(key);
        
        if (it == cache_map_.end()) {
            return -1;  // Key not found
        }
        
        // Move accessed item to front (most recently used)
        cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
        
        return it->second->second;  // Return value
    }
    
    void put(int key, int value) {
        auto it = cache_map_.find(key);
        
        if (it != cache_map_.end()) {
            // Key exists - update value and move to front
            it->second->second = value;
            cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
            return;
        }
        
        // Key doesn't exist - check capacity
        if (cache_list_.size() >= static_cast<size_t>(capacity_)) {
            // Evict least recently used (back of list)
            int lru_key = cache_list_.back().first;
            cache_list_.pop_back();
            cache_map_.erase(lru_key);
        }
        
        // Insert new item at front (most recently used)
        cache_list_.push_front({key, value});
        cache_map_[key] = cache_list_.begin();
    }
};

/**
 * Time Complexity:
 * - get(): O(1) - hash map lookup + list splice
 * - put(): O(1) - hash map lookup + list operations
 * 
 * Space Complexity: O(capacity)
 * 
 * Key Insights:
 * 1. std::list::splice() moves node in O(1) without reallocation
 * 2. Hash map stores iterators to list nodes for O(1) access
 * 3. Front of list = MRU, back of list = LRU
 * 4. Every access moves item to front
 * 5. Eviction removes from back
 * 
 * Example Usage:
 * LRUCache cache(2);  // capacity = 2
 * cache.put(1, 1);    // cache: {1=1}
 * cache.put(2, 2);    // cache: {2=2, 1=1}
 * cache.get(1);       // returns 1, cache: {1=1, 2=2}
 * cache.put(3, 3);    // evicts 2, cache: {3=3, 1=1}
 * cache.get(2);       // returns -1 (not found)
 * cache.put(4, 4);    // evicts 1, cache: {4=4, 3=3}
 * cache.get(1);       // returns -1 (not found)
 * cache.get(3);       // returns 3
 * cache.get(4);       // returns 4
 */

// Test cases
#include <cassert>
#include <iostream>

void test_lru_cache() {
    std::cout << "Testing LRU Cache..." << std::endl;
    
    LRUCache cache(2);
    
    cache.put(1, 1);
    cache.put(2, 2);
    assert(cache.get(1) == 1);       // returns 1
    
    cache.put(3, 3);                 // evicts key 2
    assert(cache.get(2) == -1);      // returns -1 (not found)
    
    cache.put(4, 4);                 // evicts key 1
    assert(cache.get(1) == -1);      // returns -1 (not found)
    assert(cache.get(3) == 3);       // returns 3
    assert(cache.get(4) == 4);       // returns 4
    
    std::cout << "All LRU Cache tests passed!" << std::endl;
}

int main() {
    test_lru_cache();
    return 0;
}
