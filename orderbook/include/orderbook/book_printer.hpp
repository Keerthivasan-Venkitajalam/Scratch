#pragma once

#include "orderbook/order_book.hpp"

namespace orderbook {

/**
 * @brief Utility class for printing order book to console
 */
class BookPrinter {
public:
    /**
     * @brief Print order book to console
     * @param book Order book to print
     * @param levels Number of levels to show on each side
     */
    static void print(const OrderBook& book, int levels = 10);
    
    /**
     * @brief Print order book with custom formatting
     * @param book Order book to print
     * @param levels Number of levels to show
     * @param show_header Whether to show column headers
     */
    static void print_detailed(const OrderBook& book, int levels = 10, bool show_header = true);
};

} // namespace orderbook
