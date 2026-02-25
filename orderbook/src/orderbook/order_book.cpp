#include "orderbook/order_book.hpp"
#include <algorithm>
#include <stdexcept>

namespace orderbook {

OrderBook::OrderBook(std::string_view symbol) 
    : symbol_(symbol) {
}

void OrderBook::add_order(Side side, int64_t price, int64_t quantity) {
    if (quantity <= 0) {
        return;  // Ignore invalid quantities
    }
    
    if (side == Side::BID) {
        auto it = bids_.find(price);
        if (it != bids_.end()) {
            // Price level exists, update it
            it->second.quantity += quantity;
            it->second.order_count++;
        } else {
            // New price level
            bids_[price] = PriceLevel(price, quantity, 1);
        }
    } else {  // ASK
        auto it = asks_.find(price);
        if (it != asks_.end()) {
            // Price level exists, update it
            it->second.quantity += quantity;
            it->second.order_count++;
        } else {
            // New price level
            asks_[price] = PriceLevel(price, quantity, 1);
        }
    }
}

void OrderBook::modify_order(Side side, int64_t price, int64_t quantity_delta) {
    if (side == Side::BID) {
        auto it = bids_.find(price);
        if (it != bids_.end()) {
            it->second.quantity += quantity_delta;
            
            // Remove price level if quantity becomes zero or negative
            if (it->second.quantity <= 0) {
                bids_.erase(it);
            }
        }
    } else {  // ASK
        auto it = asks_.find(price);
        if (it != asks_.end()) {
            it->second.quantity += quantity_delta;
            
            // Remove price level if quantity becomes zero or negative
            if (it->second.quantity <= 0) {
                asks_.erase(it);
            }
        }
    }
}

void OrderBook::delete_order(Side side, int64_t price, int64_t quantity) {
    if (quantity <= 0) {
        return;  // Ignore invalid quantities
    }
    
    if (side == Side::BID) {
        auto it = bids_.find(price);
        if (it != bids_.end()) {
            it->second.quantity -= quantity;
            if (it->second.order_count > 0) {
                it->second.order_count--;
            }
            
            // Remove price level if quantity becomes zero or negative
            if (it->second.quantity <= 0) {
                bids_.erase(it);
            }
        }
    } else {  // ASK
        auto it = asks_.find(price);
        if (it != asks_.end()) {
            it->second.quantity -= quantity;
            if (it->second.order_count > 0) {
                it->second.order_count--;
            }
            
            // Remove price level if quantity becomes zero or negative
            if (it->second.quantity <= 0) {
                asks_.erase(it);
            }
        }
    }
}

PriceLevel OrderBook::get_best_bid() const {
    if (bids_.empty()) {
        return PriceLevel();  // Empty price level
    }
    
    // std::map with std::greater: first element is highest price
    return bids_.begin()->second;
}

PriceLevel OrderBook::get_best_ask() const {
    if (asks_.empty()) {
        return PriceLevel();  // Empty price level
    }
    
    // std::map with std::less: first element is lowest price
    return asks_.begin()->second;
}

int64_t OrderBook::get_spread() const {
    if (bids_.empty() || asks_.empty()) {
        return -1;  // Invalid spread
    }
    
    int64_t best_bid_price = bids_.begin()->first;
    int64_t best_ask_price = asks_.begin()->first;
    
    return best_ask_price - best_bid_price;
}

int64_t OrderBook::get_mid_price() const {
    if (bids_.empty() || asks_.empty()) {
        return 0;  // Invalid mid price
    }
    
    int64_t best_bid_price = bids_.begin()->first;
    int64_t best_ask_price = asks_.begin()->first;
    
    return (best_bid_price + best_ask_price) / 2;
}

std::vector<PriceLevel> OrderBook::get_depth(Side side, size_t levels) const {
    std::vector<PriceLevel> depth;
    depth.reserve(levels);
    
    if (side == Side::BID) {
        size_t count = 0;
        for (const auto& [price, level] : bids_) {
            if (count >= levels) break;
            depth.push_back(level);
            count++;
        }
    } else {  // ASK
        size_t count = 0;
        for (const auto& [price, level] : asks_) {
            if (count >= levels) break;
            depth.push_back(level);
            count++;
        }
    }
    
    return depth;
}

void OrderBook::clear() {
    bids_.clear();
    asks_.clear();
}

size_t OrderBook::level_count(Side side) const {
    return (side == Side::BID) ? bids_.size() : asks_.size();
}

bool OrderBook::is_empty() const {
    return bids_.empty() && asks_.empty();
}

} // namespace orderbook
