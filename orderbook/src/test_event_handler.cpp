// Test program for OrderBookHandler
// Demonstrates event processing and order book updates

#include "orderbook/event_handler.hpp"
#include "orderbook/book_printer.hpp"

#include <iostream>
#include <iomanip>

using namespace orderbook;

void print_separator() {
    std::cout << "========================================" << std::endl;
}

void print_stats(const OrderBookHandler& handler) {
    auto stats = handler.get_stats();
    std::cout << "Event Statistics:" << std::endl;
    std::cout << "  New orders: " << stats.new_orders << std::endl;
    std::cout << "  Modifications: " << stats.modifications << std::endl;
    std::cout << "  Deletions: " << stats.deletions << std::endl;
    std::cout << "  Trades: " << stats.trades << std::endl;
    std::cout << "  Snapshots: " << stats.snapshots << std::endl;
    std::cout << "  Errors: " << stats.errors << std::endl;
}

void test_new_orders() {
    print_separator();
    std::cout << "Test 1: New Order Events" << std::endl;
    print_separator();
    
    OrderBookHandler handler("AAPL");
    
    // Add some buy orders
    NewOrderEvent buy1{1000, 1000000, "AAPL", 1, Side::BID, 150.00, 100};
    NewOrderEvent buy2{1001, 1001000, "AAPL", 2, Side::BID, 149.50, 200};
    NewOrderEvent buy3{1002, 1002000, "AAPL", 3, Side::BID, 149.00, 150};
    
    // Add some sell orders
    NewOrderEvent sell1{1003, 1003000, "AAPL", 4, Side::ASK, 150.50, 100};
    NewOrderEvent sell2{1004, 1004000, "AAPL", 5, Side::ASK, 151.00, 200};
    NewOrderEvent sell3{1005, 1005000, "AAPL", 6, Side::ASK, 151.50, 150};
    
    handler.on_new_order(buy1);
    handler.on_new_order(buy2);
    handler.on_new_order(buy3);
    handler.on_new_order(sell1);
    handler.on_new_order(sell2);
    handler.on_new_order(sell3);
    
    std::cout << "\nOrder Book after new orders:" << std::endl;
    BookPrinter::print(handler.get_order_book(), 5);
    
    std::cout << "\n";
    print_stats(handler);
    std::cout << std::endl;
}

void test_modify_orders() {
    print_separator();
    std::cout << "Test 2: Modify Order Events" << std::endl;
    print_separator();
    
    OrderBookHandler handler("GOOGL");
    
    // Add initial orders
    handler.on_new_order({1000, 1000000, "GOOGL", 1, Side::BID, 2800.00, 100});
    handler.on_new_order({1001, 1001000, "GOOGL", 2, Side::ASK, 2805.00, 100});
    
    std::cout << "\nInitial book:" << std::endl;
    BookPrinter::print(handler.get_order_book(), 3);
    
    // Modify buy order (increase quantity)
    ModifyOrderEvent modify1{2000, 2000000, "GOOGL", 1, Side::BID, 2800.00, 250, 150};
    handler.on_modify_order(modify1);
    
    std::cout << "\nAfter increasing buy quantity to 250:" << std::endl;
    BookPrinter::print(handler.get_order_book(), 3);
    
    // Modify sell order (decrease quantity)
    ModifyOrderEvent modify2{2001, 2001000, "GOOGL", 2, Side::ASK, 2805.00, 50, -50};
    handler.on_modify_order(modify2);
    
    std::cout << "\nAfter decreasing sell quantity to 50:" << std::endl;
    BookPrinter::print(handler.get_order_book(), 3);
    
    std::cout << "\n";
    print_stats(handler);
    std::cout << std::endl;
}

void test_delete_orders() {
    print_separator();
    std::cout << "Test 3: Delete Order Events" << std::endl;
    print_separator();
    
    OrderBookHandler handler("TSLA");
    
    // Build book
    handler.on_new_order({1000, 1000000, "TSLA", 1, Side::BID, 245.00, 100});
    handler.on_new_order({1001, 1001000, "TSLA", 2, Side::BID, 244.50, 200});
    handler.on_new_order({1002, 1002000, "TSLA", 3, Side::ASK, 245.50, 100});
    handler.on_new_order({1003, 1003000, "TSLA", 4, Side::ASK, 246.00, 200});
    
    std::cout << "\nInitial book:" << std::endl;
    BookPrinter::print(handler.get_order_book(), 5);
    
    // Delete best bid
    DeleteOrderEvent del1{2000, 2000000, "TSLA", 1, Side::BID, 245.00, 100};
    handler.on_delete_order(del1);
    
    std::cout << "\nAfter deleting best bid:" << std::endl;
    BookPrinter::print(handler.get_order_book(), 5);
    
    // Delete best ask
    DeleteOrderEvent del2{2001, 2001000, "TSLA", 3, Side::ASK, 245.50, 100};
    handler.on_delete_order(del2);
    
    std::cout << "\nAfter deleting best ask:" << std::endl;
    BookPrinter::print(handler.get_order_book(), 5);
    
    std::cout << "\n";
    print_stats(handler);
    std::cout << std::endl;
}

void test_trade_events() {
    print_separator();
    std::cout << "Test 4: Trade Events" << std::endl;
    print_separator();
    
    OrderBookHandler handler("MSFT");
    
    // Build book
    handler.on_new_order({1000, 1000000, "MSFT", 1, Side::BID, 380.00, 500});
    handler.on_new_order({1001, 1001000, "MSFT", 2, Side::ASK, 380.50, 300});
    
    std::cout << "\nInitial book:" << std::endl;
    BookPrinter::print(handler.get_order_book(), 3);
    
    // Buy aggressor hits ask (removes from sell side)
    TradeEvent trade1{2000, 2000000, "MSFT", 101, 0, 2, 380.50, 100, Side::BID};
    handler.on_trade(trade1);
    
    std::cout << "\nAfter buy trade (100 @ 380.50):" << std::endl;
    BookPrinter::print(handler.get_order_book(), 3);
    
    // Sell aggressor hits bid (removes from buy side)
    TradeEvent trade2{2001, 2001000, "MSFT", 102, 1, 0, 380.00, 200, Side::ASK};
    handler.on_trade(trade2);
    
    std::cout << "\nAfter sell trade (200 @ 38000):" << std::endl;
    BookPrinter::print(handler.get_order_book(), 3);
    
    std::cout << "\n";
    print_stats(handler);
    std::cout << std::endl;
}

void test_snapshot() {
    print_separator();
    std::cout << "Test 5: Snapshot Event" << std::endl;
    print_separator();
    
    OrderBookHandler handler("NVDA");
    
    // Add some initial orders
    handler.on_new_order({1000, 1000000, "NVDA", 1, Side::BID, 500.00, 100});
    handler.on_new_order({1001, 1001000, "NVDA", 2, Side::ASK, 501.00, 100});
    
    std::cout << "\nInitial book (before snapshot):" << std::endl;
    BookPrinter::print(handler.get_order_book(), 3);
    
    // Receive snapshot (replaces entire book)
    SnapshotEvent snapshot{2000, 2000000, "NVDA"};
    
    snapshot.bids = {
        {520.00, 300, 3},
        {519.50, 400, 5},
        {519.00, 200, 2}
    };
    
    snapshot.asks = {
        {520.50, 250, 2},
        {521.00, 350, 4},
        {521.50, 150, 1}
    };
    
    handler.on_snapshot(snapshot);
    
    std::cout << "\nAfter snapshot:" << std::endl;
    BookPrinter::print(handler.get_order_book(), 5);
    
    std::cout << "\n";
    print_stats(handler);
    std::cout << std::endl;
}

void test_validation() {
    print_separator();
    std::cout << "Test 6: Event Validation" << std::endl;
    print_separator();
    
    OrderBookHandler handler("TEST");
    
    // Invalid: wrong symbol
    NewOrderEvent bad1{1000, 1000000, "WRONG", 1, Side::BID, 100.00, 100};
    bool result1 = handler.on_new_order(bad1);
    std::cout << "Wrong symbol: " << (result1 ? "PASS" : "FAIL (expected)") << std::endl;
    
    // Invalid: zero price
    NewOrderEvent bad2{1001, 1001000, "TEST", 2, Side::BID, 0, 100};
    bool result2 = handler.on_new_order(bad2);
    std::cout << "Zero price: " << (result2 ? "PASS" : "FAIL (expected)") << std::endl;
    
    // Invalid: negative quantity
    NewOrderEvent bad3{1002, 1002000, "TEST", 3, Side::BID, 100.00, -100};
    bool result3 = handler.on_new_order(bad3);
    std::cout << "Negative quantity: " << (result3 ? "PASS" : "FAIL (expected)") << std::endl;
    
    // Valid order
    NewOrderEvent good{1003, 1003000, "TEST", 4, Side::BID, 100.00, 100};
    bool result4 = handler.on_new_order(good);
    std::cout << "Valid order: " << (result4 ? "PASS (expected)" : "FAIL") << std::endl;
    
    std::cout << "\n";
    print_stats(handler);
    std::cout << std::endl;
}

int main() {
    std::cout << "\n";
    print_separator();
    std::cout << "Order Book Event Handler Tests" << std::endl;
    print_separator();
    std::cout << "\n";
    
    test_new_orders();
    test_modify_orders();
    test_delete_orders();
    test_trade_events();
    test_snapshot();
    test_validation();
    
    print_separator();
    std::cout << "All tests complete!" << std::endl;
    print_separator();
    std::cout << "\n";
    
    return 0;
}
