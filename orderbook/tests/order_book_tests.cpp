#include <gtest/gtest.h>
#include "orderbook/order_book.hpp"

using namespace orderbook;

class OrderBookTest : public ::testing::Test {
protected:
    OrderBook book{"AAPL"};
    
    // Helper to convert double to fixed-point
    int64_t to_fixed(double price) {
        return static_cast<int64_t>(price * 10000);
    }
    
    // Helper to convert fixed-point to double
    double to_double(int64_t price) {
        return static_cast<double>(price) / 10000.0;
    }
};

// ============================================================================
// Basic Operations Tests
// ============================================================================

TEST_F(OrderBookTest, InitiallyEmpty) {
    EXPECT_TRUE(book.is_empty());
    EXPECT_EQ(book.level_count(Side::BID), 0);
    EXPECT_EQ(book.level_count(Side::ASK), 0);
}

TEST_F(OrderBookTest, AddSingleBidOrder) {
    book.add_order(Side::BID, to_fixed(150.00), 100);
    
    EXPECT_FALSE(book.is_empty());
    EXPECT_EQ(book.level_count(Side::BID), 1);
    
    auto best_bid = book.get_best_bid();
    EXPECT_EQ(best_bid.price, to_fixed(150.00));
    EXPECT_EQ(best_bid.quantity, 100);
    EXPECT_EQ(best_bid.order_count, 1);
}

TEST_F(OrderBookTest, AddSingleAskOrder) {
    book.add_order(Side::ASK, to_fixed(151.00), 200);
    
    EXPECT_FALSE(book.is_empty());
    EXPECT_EQ(book.level_count(Side::ASK), 1);
    
    auto best_ask = book.get_best_ask();
    EXPECT_EQ(best_ask.price, to_fixed(151.00));
    EXPECT_EQ(best_ask.quantity, 200);
    EXPECT_EQ(best_ask.order_count, 1);
}

TEST_F(OrderBookTest, AddMultipleOrdersSamePrice) {
    book.add_order(Side::BID, to_fixed(150.00), 100);
    book.add_order(Side::BID, to_fixed(150.00), 50);
    book.add_order(Side::BID, to_fixed(150.00), 75);
    
    EXPECT_EQ(book.level_count(Side::BID), 1);
    
    auto best_bid = book.get_best_bid();
    EXPECT_EQ(best_bid.price, to_fixed(150.00));
    EXPECT_EQ(best_bid.quantity, 225);  // 100 + 50 + 75
    EXPECT_EQ(best_bid.order_count, 3);
}

// ============================================================================
// Bid Side Sorting Tests (Descending Order)
// ============================================================================

TEST_F(OrderBookTest, BidsSortedDescending) {
    book.add_order(Side::BID, to_fixed(150.00), 100);
    book.add_order(Side::BID, to_fixed(149.50), 200);
    book.add_order(Side::BID, to_fixed(150.50), 150);
    
    EXPECT_EQ(book.level_count(Side::BID), 3);
    
    // Best bid should be highest price
    auto best_bid = book.get_best_bid();
    EXPECT_EQ(best_bid.price, to_fixed(150.50));
    EXPECT_EQ(best_bid.quantity, 150);
    
    // Get depth to verify sorting
    auto depth = book.get_depth(Side::BID, 3);
    ASSERT_EQ(depth.size(), 3);
    EXPECT_EQ(depth[0].price, to_fixed(150.50));  // Highest
    EXPECT_EQ(depth[1].price, to_fixed(150.00));
    EXPECT_EQ(depth[2].price, to_fixed(149.50));  // Lowest
}

// ============================================================================
// Ask Side Sorting Tests (Ascending Order)
// ============================================================================

TEST_F(OrderBookTest, AsksSortedAscending) {
    book.add_order(Side::ASK, to_fixed(151.00), 100);
    book.add_order(Side::ASK, to_fixed(151.50), 200);
    book.add_order(Side::ASK, to_fixed(150.50), 150);
    
    EXPECT_EQ(book.level_count(Side::ASK), 3);
    
    // Best ask should be lowest price
    auto best_ask = book.get_best_ask();
    EXPECT_EQ(best_ask.price, to_fixed(150.50));
    EXPECT_EQ(best_ask.quantity, 150);
    
    // Get depth to verify sorting
    auto depth = book.get_depth(Side::ASK, 3);
    ASSERT_EQ(depth.size(), 3);
    EXPECT_EQ(depth[0].price, to_fixed(150.50));  // Lowest
    EXPECT_EQ(depth[1].price, to_fixed(151.00));
    EXPECT_EQ(depth[2].price, to_fixed(151.50));  // Highest
}

// ============================================================================
// Modify Order Tests
// ============================================================================

TEST_F(OrderBookTest, ModifyOrderIncreaseQuantity) {
    book.add_order(Side::BID, to_fixed(150.00), 100);
    book.modify_order(Side::BID, to_fixed(150.00), 50);  // +50
    
    auto best_bid = book.get_best_bid();
    EXPECT_EQ(best_bid.quantity, 150);
}

TEST_F(OrderBookTest, ModifyOrderDecreaseQuantity) {
    book.add_order(Side::BID, to_fixed(150.00), 100);
    book.modify_order(Side::BID, to_fixed(150.00), -30);  // -30
    
    auto best_bid = book.get_best_bid();
    EXPECT_EQ(best_bid.quantity, 70);
}

TEST_F(OrderBookTest, ModifyOrderToZeroRemovesLevel) {
    book.add_order(Side::BID, to_fixed(150.00), 100);
    book.modify_order(Side::BID, to_fixed(150.00), -100);  // Reduce to 0
    
    EXPECT_TRUE(book.is_empty());
    EXPECT_EQ(book.level_count(Side::BID), 0);
}

// ============================================================================
// Delete Order Tests
// ============================================================================

TEST_F(OrderBookTest, DeletePartialQuantity) {
    book.add_order(Side::BID, to_fixed(150.00), 100);
    book.delete_order(Side::BID, to_fixed(150.00), 30);
    
    auto best_bid = book.get_best_bid();
    EXPECT_EQ(best_bid.quantity, 70);
}

TEST_F(OrderBookTest, DeleteEntireQuantityRemovesLevel) {
    book.add_order(Side::BID, to_fixed(150.00), 100);
    book.delete_order(Side::BID, to_fixed(150.00), 100);
    
    EXPECT_TRUE(book.is_empty());
    EXPECT_EQ(book.level_count(Side::BID), 0);
}

TEST_F(OrderBookTest, DeleteNonExistentPriceDoesNothing) {
    book.add_order(Side::BID, to_fixed(150.00), 100);
    book.delete_order(Side::BID, to_fixed(149.00), 50);  // Different price
    
    EXPECT_EQ(book.level_count(Side::BID), 1);
    auto best_bid = book.get_best_bid();
    EXPECT_EQ(best_bid.quantity, 100);  // Unchanged
}

// ============================================================================
// Spread and Mid Price Tests
// ============================================================================

TEST_F(OrderBookTest, SpreadCalculation) {
    book.add_order(Side::BID, to_fixed(150.00), 100);
    book.add_order(Side::ASK, to_fixed(151.00), 100);
    
    int64_t spread = book.get_spread();
    EXPECT_EQ(spread, to_fixed(1.00));  // 151.00 - 150.00
}

TEST_F(OrderBookTest, SpreadWithEmptyBookReturnsInvalid) {
    EXPECT_EQ(book.get_spread(), -1);
    
    book.add_order(Side::BID, to_fixed(150.00), 100);
    EXPECT_EQ(book.get_spread(), -1);  // Still invalid, no asks
}

TEST_F(OrderBookTest, MidPriceCalculation) {
    book.add_order(Side::BID, to_fixed(150.00), 100);
    book.add_order(Side::ASK, to_fixed(152.00), 100);
    
    int64_t mid = book.get_mid_price();
    EXPECT_EQ(mid, to_fixed(151.00));  // (150.00 + 152.00) / 2
}

TEST_F(OrderBookTest, MidPriceWithEmptyBookReturnsZero) {
    EXPECT_EQ(book.get_mid_price(), 0);
}

// ============================================================================
// Depth Query Tests
// ============================================================================

TEST_F(OrderBookTest, GetDepthLimitedLevels) {
    book.add_order(Side::BID, to_fixed(150.00), 100);
    book.add_order(Side::BID, to_fixed(149.50), 200);
    book.add_order(Side::BID, to_fixed(149.00), 300);
    book.add_order(Side::BID, to_fixed(148.50), 400);
    
    auto depth = book.get_depth(Side::BID, 2);  // Request only 2 levels
    
    ASSERT_EQ(depth.size(), 2);
    EXPECT_EQ(depth[0].price, to_fixed(150.00));  // Best bid
    EXPECT_EQ(depth[1].price, to_fixed(149.50));  // Second best
}

TEST_F(OrderBookTest, GetDepthMoreLevelsThanAvailable) {
    book.add_order(Side::BID, to_fixed(150.00), 100);
    book.add_order(Side::BID, to_fixed(149.50), 200);
    
    auto depth = book.get_depth(Side::BID, 10);  // Request 10, only 2 exist
    
    ASSERT_EQ(depth.size(), 2);
}

TEST_F(OrderBookTest, GetDepthEmptyBook) {
    auto depth = book.get_depth(Side::BID, 5);
    EXPECT_TRUE(depth.empty());
}

// ============================================================================
// Clear Tests
// ============================================================================

TEST_F(OrderBookTest, ClearRemovesAllOrders) {
    book.add_order(Side::BID, to_fixed(150.00), 100);
    book.add_order(Side::BID, to_fixed(149.50), 200);
    book.add_order(Side::ASK, to_fixed(151.00), 100);
    book.add_order(Side::ASK, to_fixed(151.50), 200);
    
    EXPECT_FALSE(book.is_empty());
    
    book.clear();
    
    EXPECT_TRUE(book.is_empty());
    EXPECT_EQ(book.level_count(Side::BID), 0);
    EXPECT_EQ(book.level_count(Side::ASK), 0);
}

// ============================================================================
// Complex Scenario Tests
// ============================================================================

TEST_F(OrderBookTest, RealisticMarketScenario) {
    // Build a realistic order book
    book.add_order(Side::BID, to_fixed(150.00), 500);
    book.add_order(Side::BID, to_fixed(149.95), 300);
    book.add_order(Side::BID, to_fixed(149.90), 700);
    
    book.add_order(Side::ASK, to_fixed(150.05), 400);
    book.add_order(Side::ASK, to_fixed(150.10), 600);
    book.add_order(Side::ASK, to_fixed(150.15), 200);
    
    // Verify best prices
    EXPECT_EQ(book.get_best_bid().price, to_fixed(150.00));
    EXPECT_EQ(book.get_best_ask().price, to_fixed(150.05));
    
    // Verify spread
    EXPECT_EQ(book.get_spread(), to_fixed(0.05));
    
    // Verify mid price
    EXPECT_EQ(book.get_mid_price(), to_fixed(150.025));
    
    // Simulate a trade that removes best bid
    book.delete_order(Side::BID, to_fixed(150.00), 500);
    
    // Best bid should now be 149.95
    EXPECT_EQ(book.get_best_bid().price, to_fixed(149.95));
    
    // Spread should widen
    EXPECT_EQ(book.get_spread(), to_fixed(0.10));
}

TEST_F(OrderBookTest, SymbolAccessor) {
    EXPECT_EQ(book.get_symbol(), "AAPL");
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(OrderBookTest, AddZeroQuantityIgnored) {
    book.add_order(Side::BID, to_fixed(150.00), 0);
    EXPECT_TRUE(book.is_empty());
}

TEST_F(OrderBookTest, AddNegativeQuantityIgnored) {
    book.add_order(Side::BID, to_fixed(150.00), -100);
    EXPECT_TRUE(book.is_empty());
}

TEST_F(OrderBookTest, DeleteZeroQuantityIgnored) {
    book.add_order(Side::BID, to_fixed(150.00), 100);
    book.delete_order(Side::BID, to_fixed(150.00), 0);
    
    auto best_bid = book.get_best_bid();
    EXPECT_EQ(best_bid.quantity, 100);  // Unchanged
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
