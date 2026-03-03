#pragma once

#include "orderbook/event_handler.hpp"
#include "orderbook/market_event.hpp"
#include "common/tick.hpp"

#include <memory>
#include <unordered_map>
#include <string>

namespace orderbook {

/**
 * @brief Integrates FeedHandler with OrderBook
 * 
 * Converts FIX ticks from feedhandler into market events
 * and updates order books accordingly.
 */
class FeedIntegration {
public:
    /**
     * @brief Constructor
     */
    FeedIntegration();
    
    /**
     * @brief Process a tick from the feed handler
     * @param tick Market data tick
     * @return true if successfully processed
     */
    bool process_tick(const feedhandler::common::Tick& tick);
    
    /**
     * @brief Get or create order book handler for symbol
     * @param symbol Trading symbol
     * @return Reference to order book handler
     */
    OrderBookHandler& get_handler(const std::string& symbol);
    
    /**
     * @brief Get order book for symbol
     * @param symbol Trading symbol
     * @return Pointer to order book, or nullptr if not found
     */
    OrderBook* get_order_book(const std::string& symbol);
    
    /**
     * @brief Get all active symbols
     */
    std::vector<std::string> get_symbols() const;
    
    /**
     * @brief Get processing statistics
     */
    struct Stats {
        uint64_t ticks_processed;
        uint64_t events_generated;
        uint64_t errors;
        
        Stats() : ticks_processed(0), events_generated(0), errors(0) {}
    };
    
    const Stats& get_stats() const { return stats_; }
    void reset_stats() { stats_ = Stats(); }

private:
    // Map of symbol -> OrderBookHandler
    std::unordered_map<std::string, std::unique_ptr<OrderBookHandler>> handlers_;
    
    Stats stats_;
    
    /**
     * @brief Convert tick to market event
     * @param tick Input tick
     * @return Market event (or nullptr if conversion fails)
     */
    std::unique_ptr<MarketEvent> tick_to_event(const feedhandler::common::Tick& tick);
};

} // namespace orderbook
