// Unit tests for PriceLevel struct
// Tests construction, quantity operations, comparisons, and edge cases

#include <gtest/gtest.h>
#include "orderbook/price_level.hpp"

using namespace orderbook;

// ============================================================================
// Construction Tests
// ============================================================================

TEST(PriceLevelTest, DefaultConstruction) {
    PriceLevel level;
    
    EXPECT_EQ(level.price, 0);
    EXPECT_EQ(level.quantity, 0);
    EXPECT_EQ(level.order_count, 0);
    EXPECT_TRUE(level.is_empty());
}

TEST(PriceLevelTest, ParameterizedConstruction) {
    PriceLevel level(1502500, 100, 2);
    
    EXPECT_EQ(level.price, 1502500);
    EXPECT_EQ(level.quantity, 100);
    EXPECT_EQ(level.order_count, 2);
    EXPECT_FALSE(level.is_empty());
}

TEST(PriceLevelTest, ConstructionWithDefaultOrderCount) {
    PriceLevel level(1502500, 100);
    
    EXPECT_EQ(level.price, 1502500);
    EXPECT_EQ(level.quantity, 100);
    EXPECT_EQ(level.order_count, 1);  // Default is 1
}

// ============================================================================
// Quantity Operations Tests
// ============================================================================

TEST(PriceLevelTest, AddQuantity) {
    PriceLevel level(1502500, 100, 1);
    
    level.add_quantity(50);
    
    EXPECT_EQ(level.quantity, 150);
    EXPECT_EQ(level.order_count, 2);
}

TEST(PriceLevelTest, AddQuantityMultipleOrders) {
    PriceLevel level(1502500, 100, 1);
    
    level.add_quantity(200, 3);
    
    EXPECT_EQ(level.quantity, 300);
    EXPECT_EQ(level.order_count, 4);
}

TEST(PriceLevelTest, RemoveQuantity) {
    PriceLevel level(1502500, 100, 2);
    
    bool is_empty = level.remove_quantity(30);
    
    EXPECT_EQ(level.quantity, 70);
    EXPECT_EQ(level.order_count, 1);
    EXPECT_FALSE(is_empty);
}

TEST(PriceLevelTest, RemoveQuantityMultipleOrders) {
    PriceLevel level(1502500, 300, 4);
    
    bool is_empty = level.remove_quantity(100, 2);
    
    EXPECT_EQ(level.quantity, 200);
    EXPECT_EQ(level.order_count, 2);
    EXPECT_FALSE(is_empty);
}

TEST(PriceLevelTest, RemoveQuantityUntilEmpty) {
    PriceLevel level(1502500, 100, 1);
    
    bool is_empty = level.remove_quantity(100, 1);
    
    EXPECT_EQ(level.quantity, 0);
    EXPECT_EQ(level.order_count, 0);
    EXPECT_TRUE(is_empty);
}

TEST(PriceLevelTest, RemoveMoreThanAvailable) {
    PriceLevel level(1502500, 100, 2);
    
    bool is_empty = level.remove_quantity(150, 1);
    
    EXPECT_EQ(level.quantity, -50);  // Goes negative
    EXPECT_EQ(level.order_count, 1);
    EXPECT_TRUE(is_empty);  // Considered empty when quantity <= 0
}

TEST(PriceLevelTest, RemoveMoreOrdersThanAvailable) {
    PriceLevel level(1502500, 100, 2);
    
    level.remove_quantity(50, 5);  // Try to remove 5 orders, only 2 exist
    
    EXPECT_EQ(level.quantity, 50);
    EXPECT_EQ(level.order_count, 0);  // Clamped to 0
    EXPECT_TRUE(level.is_empty());
}

// ============================================================================
// Empty State Tests
// ============================================================================

TEST(PriceLevelTest, IsEmptyWhenQuantityZero) {
    PriceLevel level(1502500, 0, 1);
    EXPECT_TRUE(level.is_empty());
}

TEST(PriceLevelTest, IsEmptyWhenOrderCountZero) {
    PriceLevel level(1502500, 100, 0);
    EXPECT_TRUE(level.is_empty());
}

TEST(PriceLevelTest, IsEmptyWhenBothZero) {
    PriceLevel level(1502500, 0, 0);
    EXPECT_TRUE(level.is_empty());
}

TEST(PriceLevelTest, IsNotEmptyWhenBothNonZero) {
    PriceLevel level(1502500, 100, 1);
    EXPECT_FALSE(level.is_empty());
}

// ============================================================================
// Average Order Size Tests
// ============================================================================

TEST(PriceLevelTest, AverageOrderSize) {
    PriceLevel level(1502500, 300, 3);
    EXPECT_EQ(level.average_order_size(), 100);
}

TEST(PriceLevelTest, AverageOrderSizeWithRemainder) {
    PriceLevel level(1502500, 250, 3);
    EXPECT_EQ(level.average_order_size(), 83);  // Integer division
}

TEST(PriceLevelTest, AverageOrderSizeZeroOrders) {
    PriceLevel level(1502500, 100, 0);
    EXPECT_EQ(level.average_order_size(), 0);  // Avoid division by zero
}

// ============================================================================
// Price Conversion Tests
// ============================================================================

TEST(PriceLevelTest, PriceAsDouble) {
    PriceLevel level(1502500, 100, 1);
    EXPECT_DOUBLE_EQ(level.price_as_double(), 150.25);
}

TEST(PriceLevelTest, PriceFromDouble) {
    int64_t price = price_from_double(150.25);
    EXPECT_EQ(price, 1502500);
}

TEST(PriceLevelTest, PriceToDouble) {
    double price = price_to_double(1502500);
    EXPECT_DOUBLE_EQ(price, 150.25);
}

TEST(PriceLevelTest, PriceConversionRoundTrip) {
    double original = 123.4567;
    int64_t fixed = price_from_double(original);
    double converted = price_to_double(fixed);
    
    EXPECT_DOUBLE_EQ(converted, original);
}

TEST(PriceLevelTest, PriceConversionZero) {
    EXPECT_EQ(price_from_double(0.0), 0);
    EXPECT_DOUBLE_EQ(price_to_double(0), 0.0);
}

TEST(PriceLevelTest, PriceConversionLargeValue) {
    double large = 9999.9999;
    int64_t fixed = price_from_double(large);
    EXPECT_EQ(fixed, 99999999);
    EXPECT_DOUBLE_EQ(price_to_double(fixed), large);
}

// ============================================================================
// Comparison Operator Tests
// ============================================================================

TEST(PriceLevelTest, EqualityOperator) {
    PriceLevel level1(1502500, 100, 2);
    PriceLevel level2(1502500, 100, 2);
    
    EXPECT_TRUE(level1 == level2);
}

TEST(PriceLevelTest, InequalityByPrice) {
    PriceLevel level1(1502500, 100, 2);
    PriceLevel level2(1502600, 100, 2);
    
    EXPECT_FALSE(level1 == level2);
}

TEST(PriceLevelTest, InequalityByQuantity) {
    PriceLevel level1(1502500, 100, 2);
    PriceLevel level2(1502500, 150, 2);
    
    EXPECT_FALSE(level1 == level2);
}

TEST(PriceLevelTest, InequalityByOrderCount) {
    PriceLevel level1(1502500, 100, 2);
    PriceLevel level2(1502500, 100, 3);
    
    EXPECT_FALSE(level1 == level2);
}

TEST(PriceLevelTest, LessThanOperator) {
    PriceLevel level1(1502500, 100, 2);
    PriceLevel level2(1502600, 100, 2);
    
    EXPECT_TRUE(level1 < level2);
    EXPECT_FALSE(level2 < level1);
}

TEST(PriceLevelTest, GreaterThanOperator) {
    PriceLevel level1(1502600, 100, 2);
    PriceLevel level2(1502500, 100, 2);
    
    EXPECT_TRUE(level1 > level2);
    EXPECT_FALSE(level2 > level1);
}

TEST(PriceLevelTest, ThreeWayComparison) {
    PriceLevel level1(1502500, 100, 2);
    PriceLevel level2(1502600, 100, 2);
    PriceLevel level3(1502500, 150, 3);  // Same price as level1
    
    EXPECT_TRUE((level1 <=> level2) < 0);
    EXPECT_TRUE((level2 <=> level1) > 0);
    EXPECT_TRUE((level1 <=> level3) == 0);  // Same price
}

// ============================================================================
// Sorting Tests
// ============================================================================

TEST(PriceLevelTest, SortAscending) {
    std::vector<PriceLevel> levels = {
        PriceLevel(1502600, 100, 1),
        PriceLevel(1502400, 200, 2),
        PriceLevel(1502500, 150, 1)
    };
    
    std::sort(levels.begin(), levels.end());
    
    EXPECT_EQ(levels[0].price, 1502400);
    EXPECT_EQ(levels[1].price, 1502500);
    EXPECT_EQ(levels[2].price, 1502600);
}

TEST(PriceLevelTest, SortDescending) {
    std::vector<PriceLevel> levels = {
        PriceLevel(1502400, 200, 2),
        PriceLevel(1502600, 100, 1),
        PriceLevel(1502500, 150, 1)
    };
    
    std::sort(levels.begin(), levels.end(), std::greater<PriceLevel>());
    
    EXPECT_EQ(levels[0].price, 1502600);
    EXPECT_EQ(levels[1].price, 1502500);
    EXPECT_EQ(levels[2].price, 1502400);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(PriceLevelTest, NegativePrice) {
    PriceLevel level(-1000, 100, 1);
    EXPECT_EQ(level.price, -1000);
    EXPECT_DOUBLE_EQ(level.price_as_double(), -0.1);
}

TEST(PriceLevelTest, ZeroQuantity) {
    PriceLevel level(1502500, 0, 1);
    EXPECT_TRUE(level.is_empty());
}

TEST(PriceLevelTest, LargeQuantity) {
    PriceLevel level(1502500, 1000000000, 1);
    EXPECT_EQ(level.quantity, 1000000000);
    EXPECT_EQ(level.average_order_size(), 1000000000);
}

TEST(PriceLevelTest, ManyOrders) {
    PriceLevel level(1502500, 1000000, 10000);
    EXPECT_EQ(level.order_count, 10000);
    EXPECT_EQ(level.average_order_size(), 100);
}

// ============================================================================
// Real-World Scenario Tests
// ============================================================================

TEST(PriceLevelTest, TypicalBidLevel) {
    // Typical bid: $150.25, 500 shares, 3 orders
    PriceLevel bid(price_from_double(150.25), 500, 3);
    
    EXPECT_DOUBLE_EQ(bid.price_as_double(), 150.25);
    EXPECT_EQ(bid.quantity, 500);
    EXPECT_EQ(bid.order_count, 3);
    EXPECT_EQ(bid.average_order_size(), 166);  // 500/3
}

TEST(PriceLevelTest, TypicalAskLevel) {
    // Typical ask: $150.26, 750 shares, 5 orders
    PriceLevel ask(price_from_double(150.26), 750, 5);
    
    EXPECT_DOUBLE_EQ(ask.price_as_double(), 150.26);
    EXPECT_EQ(ask.quantity, 750);
    EXPECT_EQ(ask.order_count, 5);
    EXPECT_EQ(ask.average_order_size(), 150);  // 750/5
}

TEST(PriceLevelTest, OrderBookUpdate) {
    // Simulate order book update sequence
    PriceLevel level(price_from_double(100.50), 0, 0);
    
    // Add first order
    level.add_quantity(100, 1);
    EXPECT_EQ(level.quantity, 100);
    EXPECT_EQ(level.order_count, 1);
    
    // Add second order
    level.add_quantity(200, 1);
    EXPECT_EQ(level.quantity, 300);
    EXPECT_EQ(level.order_count, 2);
    
    // Remove first order
    level.remove_quantity(100, 1);
    EXPECT_EQ(level.quantity, 200);
    EXPECT_EQ(level.order_count, 1);
    
    // Remove second order
    bool empty = level.remove_quantity(200, 1);
    EXPECT_TRUE(empty);
    EXPECT_EQ(level.quantity, 0);
    EXPECT_EQ(level.order_count, 0);
}
