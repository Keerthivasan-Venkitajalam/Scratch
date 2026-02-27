// Test program for market depth query functionality
// Demonstrates querying top N price levels from order book

#include "orderbook/order_book.hpp"
#include <iostream>
#include <iomanip>

using namespace orderbook;

// Helper to convert fixed-point price to double
double price_to_double(int64_t price) {
    return static_cast<double>(price) / 10000.0;
}

// Helper to convert double to fixed-point price
int64_t double_to_price(double price) {
    return static_cast<int64_t>(price * 10000.0);
}

void print_separator() {
    std::cout << "========================================" << std::endl;
}

void print_depth(const std::vector<PriceLevel>& depth, Side side) {
    std::string side_name = (side == Side::BID) ? "BID" : "ASK";
    
    std::cout << side_name << " Depth:" << std::endl;
    std::cout << std::setw(12) << "Price" 
              << std::setw(12) << "Quantity" 
              << std::setw(12) << "Orders" << std::endl;
    std::cout << std::string(36, '-') << std::endl;
    
    for (const auto& level : depth) {
        std::cout << std::fixed << std::setprecision(2)
                  << std::setw(12) << price_to_double(level.price)
                  << std::setw(12) << level.quantity
                  << std::setw(12) << level.order_count << std::endl;
    }
    std::cout << std::endl;
}

void test_basic_depth_query() {
    print_separator();
    std::cout << "Test 1: Basic Depth Query" << std::endl;
    print_separator();
    
    OrderBook book("AAPL");
    
    // Add bid orders at different prices
    book.add_order(Side::BID, double_to_price(150.00), 100);
    book.add_order(Side::BID, double_to_price(149.95), 200);
    book.add_order(Side::BID, double_to_price(149.90), 150);
    book.add_order(Side::BID, double_to_price(149.85), 300);
    book.add_order(Side::BID, double_to_price(149.80), 250);
    
    // Add ask orders at different prices
    book.add_order(Side::ASK, double_to_price(150.05), 120);
    book.add_order(Side::ASK, double_to_price(150.10), 180);
    book.add_order(Side::ASK, double_to_price(150.15), 160);
    book.add_order(Side::ASK, double_to_price(150.20), 220);
    book.add_order(Side::ASK, double_to_price(150.25), 190);
    
    // Query top 3 levels
    auto bid_depth = book.get_depth(Side::BID, 3);
    auto ask_depth = book.get_depth(Side::ASK, 3);
    
    std::cout << "Querying top 3 levels:" << std::endl << std::endl;
    print_depth(bid_depth, Side::BID);
    print_depth(ask_depth, Side::ASK);
    
    std::cout << "Best Bid: " << price_to_double(book.get_best_bid().price) << std::endl;
    std::cout << "Best Ask: " << price_to_double(book.get_best_ask().price) << std::endl;
    std::cout << "Spread: " << price_to_double(book.get_spread()) << std::endl;
    std::cout << std::endl;
}

void test_full_depth() {
    print_separator();
    std::cout << "Test 2: Full Depth Query" << std::endl;
    print_separator();
    
    OrderBook book("MSFT");
    
    // Add 10 bid levels
    for (int i = 0; i < 10; ++i) {
        double price = 380.00 - (i * 0.05);
        int64_t qty = 100 + (i * 50);
        book.add_order(Side::BID, double_to_price(price), qty);
    }
    
    // Add 10 ask levels
    for (int i = 0; i < 10; ++i) {
        double price = 380.05 + (i * 0.05);
        int64_t qty = 120 + (i * 40);
        book.add_order(Side::ASK, double_to_price(price), qty);
    }
    
    // Query all levels (more than available)
    auto bid_depth = book.get_depth(Side::BID, 100);
    auto ask_depth = book.get_depth(Side::ASK, 100);
    
    std::cout << "Querying all levels (requested 100, have 10 each):" << std::endl << std::endl;
    print_depth(bid_depth, Side::BID);
    print_depth(ask_depth, Side::ASK);
    
    std::cout << "Total bid levels: " << bid_depth.size() << std::endl;
    std::cout << "Total ask levels: " << ask_depth.size() << std::endl;
    std::cout << std::endl;
}

void test_aggregated_levels() {
    print_separator();
    std::cout << "Test 3: Aggregated Price Levels" << std::endl;
    print_separator();
    
    OrderBook book("GOOGL");
    
    // Add multiple orders at same price (should aggregate)
    book.add_order(Side::BID, double_to_price(2800.00), 50);
    book.add_order(Side::BID, double_to_price(2800.00), 75);
    book.add_order(Side::BID, double_to_price(2800.00), 100);
    
    book.add_order(Side::BID, double_to_price(2799.95), 200);
    book.add_order(Side::BID, double_to_price(2799.90), 150);
    
    book.add_order(Side::ASK, double_to_price(2800.05), 60);
    book.add_order(Side::ASK, double_to_price(2800.05), 80);
    book.add_order(Side::ASK, double_to_price(2800.10), 120);
    
    auto bid_depth = book.get_depth(Side::BID, 5);
    auto ask_depth = book.get_depth(Side::ASK, 5);
    
    std::cout << "Multiple orders at same price aggregate:" << std::endl << std::endl;
    print_depth(bid_depth, Side::BID);
    print_depth(ask_depth, Side::ASK);
    
    std::cout << "Note: 3 orders at 2800.00 aggregated to qty=225, orders=3" << std::endl;
    std::cout << std::endl;
}

void test_empty_book() {
    print_separator();
    std::cout << "Test 4: Empty Book Depth Query" << std::endl;
    print_separator();
    
    OrderBook book("TSLA");
    
    auto bid_depth = book.get_depth(Side::BID, 10);
    auto ask_depth = book.get_depth(Side::ASK, 10);
    
    std::cout << "Querying empty book:" << std::endl;
    std::cout << "Bid levels returned: " << bid_depth.size() << std::endl;
    std::cout << "Ask levels returned: " << ask_depth.size() << std::endl;
    std::cout << std::endl;
}

void test_depth_after_modifications() {
    print_separator();
    std::cout << "Test 5: Depth After Modifications" << std::endl;
    print_separator();
    
    OrderBook book("NVDA");
    
    // Initial orders
    book.add_order(Side::BID, double_to_price(500.00), 100);
    book.add_order(Side::BID, double_to_price(499.95), 200);
    book.add_order(Side::BID, double_to_price(499.90), 150);
    
    std::cout << "Initial depth:" << std::endl;
    auto depth1 = book.get_depth(Side::BID, 5);
    print_depth(depth1, Side::BID);
    
    // Modify top level
    book.modify_order(Side::BID, double_to_price(500.00), 50);  // Add 50
    
    std::cout << "After adding 50 to top level:" << std::endl;
    auto depth2 = book.get_depth(Side::BID, 5);
    print_depth(depth2, Side::BID);
    
    // Delete from middle level
    book.delete_order(Side::BID, double_to_price(499.95), 200);  // Remove all
    
    std::cout << "After deleting middle level:" << std::endl;
    auto depth3 = book.get_depth(Side::BID, 5);
    print_depth(depth3, Side::BID);
}

void test_total_liquidity() {
    print_separator();
    std::cout << "Test 6: Total Liquidity Calculation" << std::endl;
    print_separator();
    
    OrderBook book("AMZN");
    
    // Add orders
    book.add_order(Side::BID, double_to_price(180.00), 100);
    book.add_order(Side::BID, double_to_price(179.95), 200);
    book.add_order(Side::BID, double_to_price(179.90), 300);
    book.add_order(Side::BID, double_to_price(179.85), 400);
    book.add_order(Side::BID, double_to_price(179.80), 500);
    
    auto depth = book.get_depth(Side::BID, 5);
    
    // Calculate total liquidity
    int64_t total_qty = 0;
    uint32_t total_orders = 0;
    
    for (const auto& level : depth) {
        total_qty += level.quantity;
        total_orders += level.order_count;
    }
    
    std::cout << "Top 5 bid levels:" << std::endl;
    print_depth(depth, Side::BID);
    
    std::cout << "Total liquidity in top 5 levels:" << std::endl;
    std::cout << "  Total quantity: " << total_qty << std::endl;
    std::cout << "  Total orders: " << total_orders << std::endl;
    std::cout << std::endl;
}

void test_performance() {
    print_separator();
    std::cout << "Test 7: Performance Test" << std::endl;
    print_separator();
    
    OrderBook book("SPY");
    
    // Add 1000 price levels
    std::cout << "Adding 1000 price levels..." << std::endl;
    for (int i = 0; i < 1000; ++i) {
        double price = 450.00 - (i * 0.01);
        book.add_order(Side::BID, double_to_price(price), 100);
    }
    
    std::cout << "Total levels: " << book.level_count(Side::BID) << std::endl;
    
    // Query top 10 (should be O(10) regardless of total levels)
    auto depth = book.get_depth(Side::BID, 10);
    
    std::cout << "Queried top 10 levels:" << std::endl;
    print_depth(depth, Side::BID);
    
    std::cout << "Note: Query is O(k) where k=10, not O(n) where n=1000" << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "\n";
    print_separator();
    std::cout << "Market Depth Query Tests" << std::endl;
    print_separator();
    std::cout << "\n";
    
    test_basic_depth_query();
    test_full_depth();
    test_aggregated_levels();
    test_empty_book();
    test_depth_after_modifications();
    test_total_liquidity();
    test_performance();
    
    print_separator();
    std::cout << "All tests complete!" << std::endl;
    print_separator();
    std::cout << "\n";
    
    return 0;
}
