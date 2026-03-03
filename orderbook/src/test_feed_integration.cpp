// Test FeedHandler -> OrderBook integration
// Demonstrates converting FIX ticks to order book updates

#include "orderbook/feed_integration.hpp"
#include "orderbook/book_printer.hpp"
#include "common/tick.hpp"

#include <iostream>
#include <vector>

using namespace orderbook;
using namespace feedhandler::common;

void print_separator() {
    std::cout << "========================================" << std::endl;
}

void create_sample_ticks(std::vector<Tick>& ticks) {
    // Create some sample ticks for AAPL
    Tick tick1;
    tick1.copy_symbol("AAPL");
    tick1.price = double_to_price(150.00);
    tick1.qty = 100;
    tick1.side = 'B';  // Buy
    tick1.timestamp = 1000000;
    ticks.push_back(tick1);
    
    Tick tick2;
    tick2.copy_symbol("AAPL");
    tick2.price = double_to_price(149.50);
    tick2.qty = 200;
    tick2.side = 'B';
    tick2.timestamp = 1001000;
    ticks.push_back(tick2);
    
    Tick tick3;
    tick3.copy_symbol("AAPL");
    tick3.price = double_to_price(150.50);
    tick3.qty = 150;
    tick3.side = 'S';  // Sell
    tick3.timestamp = 1002000;
    ticks.push_back(tick3);
    
    Tick tick4;
    tick4.copy_symbol("AAPL");
    tick4.price = double_to_price(151.00);
    tick4.qty = 100;
    tick4.side = 'S';
    tick4.timestamp = 1003000;
    ticks.push_back(tick4);
    
    // Add ticks for GOOGL
    Tick tick5;
    tick5.copy_symbol("GOOGL");
    tick5.price = double_to_price(2800.00);
    tick5.qty = 50;
    tick5.side = 'B';
    tick5.timestamp = 1004000;
    ticks.push_back(tick5);
    
    Tick tick6;
    tick6.copy_symbol("GOOGL");
    tick6.price = double_to_price(2805.00);
    tick6.qty = 75;
    tick6.side = 'S';
    tick6.timestamp = 1005000;
    ticks.push_back(tick6);
}

int main() {
    std::cout << "\n";
    print_separator();
    std::cout << "FeedHandler -> OrderBook Integration Test" << std::endl;
    print_separator();
    std::cout << "\n";
    
    // Create feed integration
    FeedIntegration integration;
    
    // Create sample ticks
    std::vector<Tick> ticks;
    create_sample_ticks(ticks);
    
    std::cout << "Processing " << ticks.size() << " ticks..." << std::endl;
    std::cout << "\n";
    
    // Process each tick
    for (const auto& tick : ticks) {
        std::string symbol(tick.symbol.data(), tick.symbol.size());
        double price = price_to_double(tick.price);
        
        std::cout << "Tick: " << symbol 
                  << " | Side: " << tick.side
                  << " | Price: " << price
                  << " | Qty: " << tick.qty
                  << std::endl;
        
        bool success = integration.process_tick(tick);
        if (!success) {
            std::cerr << "  ERROR: Failed to process tick" << std::endl;
        }
    }
    
    std::cout << "\n";
    print_separator();
    std::cout << "Processing Statistics" << std::endl;
    print_separator();
    
    auto stats = integration.get_stats();
    std::cout << "Ticks processed: " << stats.ticks_processed << std::endl;
    std::cout << "Events generated: " << stats.events_generated << std::endl;
    std::cout << "Errors: " << stats.errors << std::endl;
    std::cout << "\n";
    
    // Display order books
    auto symbols = integration.get_symbols();
    std::cout << "Active symbols: " << symbols.size() << std::endl;
    std::cout << "\n";
    
    for (const auto& symbol : symbols) {
        print_separator();
        std::cout << "Order Book: " << symbol << std::endl;
        print_separator();
        
        auto* book = integration.get_order_book(symbol);
        if (book) {
            BookPrinter::print(*book, 5);
        }
        std::cout << "\n";
    }
    
    print_separator();
    std::cout << "Integration test complete!" << std::endl;
    print_separator();
    std::cout << "\n";
    
    return 0;
}
