// Order Book Visualization - Console Printer
// Displays order book in human-readable format with bid/ask spread

#include "orderbook/order_book.hpp"
#include "orderbook/price_level.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace orderbook {

class BookPrinter {
public:
    /**
     * @brief Print order book to console with formatting
     * @param book Order book to print
     * @param levels Number of levels to show (default 10)
     */
    static void print(const OrderBook& book, size_t levels = 10) {
        print_header(book.get_symbol());
        
        // Get market depth
        auto asks = book.get_depth(Side::ASK, levels);
        auto bids = book.get_depth(Side::BID, levels);
        
        // Print asks (reversed - highest first)
        print_asks(asks);
        
        // Print spread
        print_spread(book);
        
        // Print bids (highest first)
        print_bids(bids);
        
        print_footer();
    }
    
    /**
     * @brief Print compact one-line summary
     */
    static void print_summary(const OrderBook& book) {
        auto best_bid = book.get_best_bid();
        auto best_ask = book.get_best_ask();
        
        std::cout << book.get_symbol() << " | ";
        
        if (best_bid.price > 0) {
            std::cout << "Bid: " << format_price(best_bid.price) 
                     << " (" << best_bid.quantity << ") ";
        } else {
            std::cout << "Bid: --- ";
        }
        
        std::cout << "| ";
        
        if (best_ask.price > 0) {
            std::cout << "Ask: " << format_price(best_ask.price) 
                     << " (" << best_ask.quantity << ") ";
        } else {
            std::cout << "Ask: --- ";
        }
        
        if (best_bid.price > 0 && best_ask.price > 0) {
            int64_t spread = best_ask.price - best_bid.price;
            int64_t mid = (best_bid.price + best_ask.price) / 2;
            double spread_bps = (static_cast<double>(spread) / mid) * 10000.0;
            std::cout << "| Spread: " << format_price(spread) 
                     << " (" << std::fixed << std::setprecision(2) << spread_bps << " bps)";
        }
        
        std::cout << std::endl;
    }

private:
    static void print_header(std::string_view symbol) {
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  ORDER BOOK: " << std::left << std::setw(43) << symbol << "║\n";
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
        std::cout << "║     PRICE     │    QUANTITY    │   ORDERS   │    SIDE    ║\n";
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    }
    
    static void print_asks(const std::vector<PriceLevel>& asks) {
        // Print asks in reverse order (highest to lowest)
        for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
            print_level(*it, Side::ASK);
        }
    }
    
    static void print_bids(const std::vector<PriceLevel>& bids) {
        // Print bids in order (highest to lowest)
        for (const auto& level : bids) {
            print_level(level, Side::BID);
        }
    }
    
    static void print_level(const PriceLevel& level, Side side) {
        std::cout << "║ ";
        
        // Price (right-aligned)
        std::cout << std::right << std::setw(13) << format_price(level.price) << " │ ";
        
        // Quantity (right-aligned)
        std::cout << std::right << std::setw(14) << level.quantity << " │ ";
        
        // Order count (right-aligned)
        std::cout << std::right << std::setw(10) << level.order_count << " │ ";
        
        // Side with color indicator
        if (side == Side::BID) {
            std::cout << "   \033[32mBID\033[0m     ";  // Green
        } else {
            std::cout << "   \033[31mASK\033[0m     ";  // Red
        }
        
        std::cout << "║\n";
    }
    
    static void print_spread(const OrderBook& book) {
        auto best_bid = book.get_best_bid();
        auto best_ask = book.get_best_ask();
        
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
        
        if (best_bid.price > 0 && best_ask.price > 0) {
            int64_t spread = best_ask.price - best_bid.price;
            int64_t mid = (best_bid.price + best_ask.price) / 2;
            double spread_bps = (static_cast<double>(spread) / mid) * 10000.0;
            
            std::cout << "║  SPREAD: " << std::left << std::setw(12) << format_price(spread)
                     << " │ MID: " << std::left << std::setw(12) << format_price(mid)
                     << " │ " << std::fixed << std::setprecision(2) << std::setw(6) << spread_bps 
                     << " bps  ║\n";
        } else {
            std::cout << "║  SPREAD: ---           │ MID: ---           │ --- bps    ║\n";
        }
        
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    }
    
    static void print_footer() {
        std::cout << "╚════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
    }
    
    static std::string format_price(int64_t price_fixed) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << price_to_double(price_fixed);
        return oss.str();
    }
};

} // namespace orderbook

// Test program
int main() {
    using namespace orderbook;
    
    std::cout << "Order Book Visualization Demo\n";
    std::cout << "==============================\n";
    
    // Create order book
    OrderBook book("AAPL");
    
    // Add some orders to create a realistic book
    // Bids (buy orders)
    book.add_order(Side::BID, price_from_double(150.00), 1000);
    book.add_order(Side::BID, price_from_double(149.99), 1500);
    book.add_order(Side::BID, price_from_double(149.98), 2000);
    book.add_order(Side::BID, price_from_double(149.97), 1200);
    book.add_order(Side::BID, price_from_double(149.96), 800);
    book.add_order(Side::BID, price_from_double(149.95), 1800);
    book.add_order(Side::BID, price_from_double(149.94), 900);
    book.add_order(Side::BID, price_from_double(149.93), 1100);
    book.add_order(Side::BID, price_from_double(149.92), 1300);
    book.add_order(Side::BID, price_from_double(149.91), 700);
    
    // Asks (sell orders)
    book.add_order(Side::ASK, price_from_double(150.01), 900);
    book.add_order(Side::ASK, price_from_double(150.02), 1400);
    book.add_order(Side::ASK, price_from_double(150.03), 1900);
    book.add_order(Side::ASK, price_from_double(150.04), 1100);
    book.add_order(Side::ASK, price_from_double(150.05), 750);
    book.add_order(Side::ASK, price_from_double(150.06), 1600);
    book.add_order(Side::ASK, price_from_double(150.07), 850);
    book.add_order(Side::ASK, price_from_double(150.08), 1050);
    book.add_order(Side::ASK, price_from_double(150.09), 1250);
    book.add_order(Side::ASK, price_from_double(150.10), 650);
    
    // Print full book
    BookPrinter::print(book, 10);
    
    // Print summary
    std::cout << "Summary View:\n";
    std::cout << "-------------\n";
    BookPrinter::print_summary(book);
    
    // Simulate some trades
    std::cout << "\nAfter executing 500 shares at best ask...\n";
    book.modify_order(Side::ASK, price_from_double(150.01), -500);
    BookPrinter::print_summary(book);
    
    std::cout << "\nAfter adding large bid...\n";
    book.add_order(Side::BID, price_from_double(149.99), 5000);
    BookPrinter::print_summary(book);
    
    // Show top 5 levels
    std::cout << "\nTop 5 Levels:\n";
    BookPrinter::print(book, 5);
    
    return 0;
}
