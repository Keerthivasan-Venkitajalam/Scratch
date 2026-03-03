// LeetCode 355: Design Twitter
// Problem: Design a simplified Twitter with post, follow, and getNewsFeed operations
//
// Operations:
// - postTweet(userId, tweetId): User posts a tweet
// - getNewsFeed(userId): Get 10 most recent tweets from user and followees
// - follow(followerId, followeeId): User follows another user
// - unfollow(followerId, followeeId): User unfollows another user
//
// Approach: Use hash maps and priority queue for efficient feed generation
// Time: postTweet O(1), follow/unfollow O(1), getNewsFeed O(N log k) where N = total tweets, k = 10

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>

class Twitter {
private:
    struct Tweet {
        int tweetId;
        int timestamp;
        
        Tweet(int id, int time) : tweetId(id), timestamp(time) {}
        
        // For priority queue (max heap by timestamp)
        bool operator<(const Tweet& other) const {
            return timestamp < other.timestamp;
        }
    };
    
    int globalTime;
    std::unordered_map<int, std::vector<Tweet>> userTweets;  // userId -> tweets
    std::unordered_map<int, std::unordered_set<int>> following;  // userId -> followees
    
public:
    Twitter() : globalTime(0) {
    }
    
    void postTweet(int userId, int tweetId) {
        userTweets[userId].emplace_back(tweetId, globalTime++);
    }
    
    std::vector<int> getNewsFeed(int userId) {
        std::priority_queue<Tweet> pq;
        
        // Add user's own tweets
        if (userTweets.count(userId)) {
            for (const auto& tweet : userTweets[userId]) {
                pq.push(tweet);
            }
        }
        
        // Add followees' tweets
        if (following.count(userId)) {
            for (int followeeId : following[userId]) {
                if (userTweets.count(followeeId)) {
                    for (const auto& tweet : userTweets[followeeId]) {
                        pq.push(tweet);
                    }
                }
            }
        }
        
        // Get top 10 most recent
        std::vector<int> feed;
        int count = 0;
        while (!pq.empty() && count < 10) {
            feed.push_back(pq.top().tweetId);
            pq.pop();
            count++;
        }
        
        return feed;
    }
    
    void follow(int followerId, int followeeId) {
        if (followerId != followeeId) {  // Can't follow yourself
            following[followerId].insert(followeeId);
        }
    }
    
    void unfollow(int followerId, int followeeId) {
        if (following.count(followerId)) {
            following[followerId].erase(followeeId);
        }
    }
};

// Optimized version using merge k sorted lists approach:
class TwitterOptimized {
private:
    struct Tweet {
        int tweetId;
        int timestamp;
        Tweet* next;
        
        Tweet(int id, int time) : tweetId(id), timestamp(time), next(nullptr) {}
    };
    
    struct TweetComparator {
        bool operator()(const Tweet* a, const Tweet* b) const {
            return a->timestamp < b->timestamp;  // Max heap
        }
    };
    
    int globalTime;
    std::unordered_map<int, Tweet*> userTweetHead;  // userId -> head of tweet list
    std::unordered_map<int, std::unordered_set<int>> following;
    
public:
    TwitterOptimized() : globalTime(0) {
    }
    
    ~TwitterOptimized() {
        // Clean up tweet lists
        for (auto& pair : userTweetHead) {
            Tweet* curr = pair.second;
            while (curr) {
                Tweet* next = curr->next;
                delete curr;
                curr = next;
            }
        }
    }
    
    void postTweet(int userId, int tweetId) {
        Tweet* newTweet = new Tweet(tweetId, globalTime++);
        newTweet->next = userTweetHead[userId];
        userTweetHead[userId] = newTweet;
    }
    
    std::vector<int> getNewsFeed(int userId) {
        std::priority_queue<Tweet*, std::vector<Tweet*>, TweetComparator> pq;
        
        // Add user's most recent tweet
        if (userTweetHead.count(userId) && userTweetHead[userId]) {
            pq.push(userTweetHead[userId]);
        }
        
        // Add each followee's most recent tweet
        if (following.count(userId)) {
            for (int followeeId : following[userId]) {
                if (userTweetHead.count(followeeId) && userTweetHead[followeeId]) {
                    pq.push(userTweetHead[followeeId]);
                }
            }
        }
        
        // Merge k sorted lists (each user's tweets are sorted by time)
        std::vector<int> feed;
        while (!pq.empty() && feed.size() < 10) {
            Tweet* top = pq.top();
            pq.pop();
            
            feed.push_back(top->tweetId);
            
            // Add next tweet from same user
            if (top->next) {
                pq.push(top->next);
            }
        }
        
        return feed;
    }
    
    void follow(int followerId, int followeeId) {
        if (followerId != followeeId) {
            following[followerId].insert(followeeId);
        }
    }
    
    void unfollow(int followerId, int followeeId) {
        if (following.count(followerId)) {
            following[followerId].erase(followeeId);
        }
    }
};

// Application to trading:
// - Similar to order book feed aggregation
// - Merging multiple market data feeds
// - Maintaining sorted order of events from multiple sources
// - Priority queue for event processing

