#pragma once

#include <cstdint>
#include <compare>

namespace orderbook {

/**
 * @brief Price level in the order book
 * 
 * Aggregates all orders at a specific price point.
 * Uses fixed-point arithmetic for price (scaled by 10000).
 * 
 * Example: $150.25 = 1502500 (150.25 * 10000)
 */
struct PriceLevel {
    int64_t price;        // Price in fixed-point (scaled by 10000)
    int64_t quantity;     // Total quantity at this price
    uint32_t order_count; // Number of orders at this price
    
    /**
     * @brief Default constructor
     */
    PriceLevel() : price(0), quantity(0), order_count(0) {}
    
    /**
     * @brief Constructor with values
     */
    PriceLevel(int64_t p, int64_t q, uint32_t c = 1) 
        : price(p), quantity(q), order_count(c) {}
    
    /**
     * @brief Add quantity to this level
     * @param qty Quantity to add
     * @param orders Number of orders to add (default 1)
     */
    void add_quantity(int64_t qty, uint32_t orders = 1) {
        quantity += qty;
        order_count += orders;
    }
    
    /**
     * @brief Remove quantity from this level
     * @param qty Quantity to remove
     * @param orders Number of orders to remove (default 1)
     * @return true if level is now empty (quantity <= 0)
     */
    bool remove_quantity(int64_t qty, uint32_t orders = 1) {
        quantity -= qty;
        if (order_count >= orders) {
            order_count -= orders;
        } else {
            order_count = 0;
        }
        return quantity <= 0;
    }
    
    /**
     * @brief Check if level is empty
     */
    bool is_empty() const {
        return quantity <= 0 || order_count == 0;
    }
    
    /**
     * @brief Get average order size at this level
     */
    int64_t average_order_size() const {
        return order_count > 0 ? quantity / order_count : 0;
    }
    
    /**
     * @brief Convert price to double
     */
    double price_as_double() const {
        return static_cast<double>(price) / 10000.0;
    }
    
    // Comparison operators for sorting
    
    /**
     * @brief Three-way comparison operator (C++20)
     * Compares by price only
     */
    auto operator<=>(const PriceLevel& other) const {
        return price <=> other.price;
    }
    
    /**
     * @brief Equality comparison
     * Compares all fields
     */
    bool operator==(const PriceLevel& other) const {
        return price == other.price && 
               quantity == other.quantity && 
               order_count == other.order_count;
    }
    
    /**
     * @brief Less than comparison (by price)
     */
    bool operator<(const PriceLevel& other) const {
        return price < other.price;
    }
    
    /**
     * @brief Greater than comparison (by price)
     */
    bool operator>(const PriceLevel& other) const {
        return price > other.price;
    }
};

/**
 * @brief Helper function to create price from double
 * @param price_double Price as double (e.g., 150.25)
 * @return Fixed-point price (e.g., 1502500)
 */
inline int64_t price_from_double(double price_double) {
    return static_cast<int64_t>(price_double * 10000.0);
}

/**
 * @brief Helper function to convert price to double
 * @param price_fixed Fixed-point price (e.g., 1502500)
 * @return Price as double (e.g., 150.25)
 */
inline double price_to_double(int64_t price_fixed) {
    return static_cast<double>(price_fixed) / 10000.0;
}

} // namespace orderbook
