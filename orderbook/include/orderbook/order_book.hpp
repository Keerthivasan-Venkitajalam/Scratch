#pragma once

#include <map>
#include <vector>
#include <cstdint>
#include <string_view>

namespace orderbook {

/**
 * @brief Price level in the order book
 * Aggregates all orders at a specific price point
 */
struct PriceLevel {
    int64_t price;        // Price in fixed-point (scaled by 10000)
    int64_t quantity;     // Total quantity at this price
    uint32_t order_count; // Number of orders at this price
    
    PriceLevel() : price(0), quantity(0), order_count(0) {}
    PriceLevel(int64_t p, int64_t q, uint32_t c) 
        : price(p), quantity(q), order_count(c) {}
};

/**
 * @brief Side of the order book
 */
enum class Side {
    BID,  // Buy side
    ASK   // Sell side
};

/**
 * @brief Limit order book maintaining real-time market depth
 * 
 * Data structure choice: std::map
 * - Bid side: std::map with std::greater (descending, highest price first)
 * - Ask side: std::map with std::less (ascending, lowest price first)
 * 
 * Complexity:
 * - Insert: O(log n)
 * - Update: O(log n)
 * - Delete: O(log n)
 * - Best bid/ask: O(1)
 * - Get depth: O(k) where k = number of levels
 * 
 * Memory: O(n) where n = number of price levels
 */
class OrderBook {
public:
    /**
     * @brief Constructor
     * @param symbol Trading symbol (e.g., "AAPL", "MSFT")
     */
    explicit OrderBook(std::string_view symbol);
    
    /**
     * @brief Add a new order to the book
     * @param side BID or ASK
     * @param price Order price (fixed-point)
     * @param quantity Order quantity
     */
    void add_order(Side side, int64_t price, int64_t quantity);
    
    /**
     * @brief Modify existing order quantity at a price level
     * @param side BID or ASK
     * @param price Price level
     * @param quantity_delta Change in quantity (can be negative)
     */
    void modify_order(Side side, int64_t price, int64_t quantity_delta);
    
    /**
     * @brief Delete order(s) at a price level
     * @param side BID or ASK
     * @param price Price level
     * @param quantity Quantity to remove
     */
    void delete_order(Side side, int64_t price, int64_t quantity);
    
    /**
     * @brief Get best bid price and quantity
     * @return PriceLevel with best bid, or empty if no bids
     */
    PriceLevel get_best_bid() const;
    
    /**
     * @brief Get best ask price and quantity
     * @return PriceLevel with best ask, or empty if no asks
     */
    PriceLevel get_best_ask() const;
    
    /**
     * @brief Get bid-ask spread
     * @return Spread in fixed-point, or -1 if book is empty
     */
    int64_t get_spread() const;
    
    /**
     * @brief Get mid price (average of best bid and ask)
     * @return Mid price in fixed-point, or 0 if book is empty
     */
    int64_t get_mid_price() const;
    
    /**
     * @brief Get market depth for a side
     * @param side BID or ASK
     * @param levels Number of price levels to return
     * @return Vector of price levels, sorted by price
     */
    std::vector<PriceLevel> get_depth(Side side, size_t levels) const;
    
    /**
     * @brief Clear all orders from the book
     */
    void clear();
    
    /**
     * @brief Get total number of price levels
     * @param side BID or ASK
     * @return Number of price levels on the specified side
     */
    size_t level_count(Side side) const;
    
    /**
     * @brief Get trading symbol
     */
    std::string_view get_symbol() const { return symbol_; }
    
    /**
     * @brief Check if book is empty
     */
    bool is_empty() const;

private:
    std::string symbol_;
    
    // Bid side: descending order (highest price first)
    // Key = price, Value = PriceLevel
    std::map<int64_t, PriceLevel, std::greater<int64_t>> bids_;
    
    // Ask side: ascending order (lowest price first)
    std::map<int64_t, PriceLevel, std::less<int64_t>> asks_;
    
    /**
     * @brief Get the appropriate map for a side
     */
    std::map<int64_t, PriceLevel, std::greater<int64_t>>& get_side_map(Side side);
    const std::map<int64_t, PriceLevel, std::greater<int64_t>>& get_side_map(Side side) const;
};

} // namespace orderbook
