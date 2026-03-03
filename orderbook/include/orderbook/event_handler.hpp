#pragma once

#include "orderbook/order_book.hpp"
#include "orderbook/market_event.hpp"

#include <memory>
#include <string>

namespace orderbook {

/**
 * @brief Event handler that processes market data events and updates order book
 * 
 * This class acts as the bridge between market data feed and order book.
 * It receives market events (new order, modify, delete, trade) and applies
 * them to the order book, maintaining consistency and handling edge cases.
 */
class OrderBookHandler {
public:
    /**
     * @brief Constructor
     * @param symbol Symbol for this order book
     */
    explicit OrderBookHandler(const std::string& symbol);
    
    /**
     * @brief Get the order book
     */
    OrderBook& get_order_book() { return order_book_; }
    const OrderBook& get_order_book() const { return order_book_; }
    
    /**
     * @brief Handle new order event
     * @param event New order event
     * @return true if successfully processed
     */
    bool on_new_order(const NewOrderEvent& event);
    
    /**
     * @brief Handle modify order event
     * @param event Modify order event
     * @return true if successfully processed
     */
    bool on_modify_order(const ModifyOrderEvent& event);
    
    /**
     * @brief Handle delete order event
     * @param event Delete order event
     * @return true if successfully processed
     */
    bool on_delete_order(const DeleteOrderEvent& event);
    
    /**
     * @brief Handle trade event
     * @param event Trade event
     * @return true if successfully processed
     */
    bool on_trade(const TradeEvent& event);
    
    /**
     * @brief Handle snapshot event (full book rebuild)
     * @param event Snapshot event
     * @return true if successfully processed
     */
    bool on_snapshot(const SnapshotEvent& event);
    
    /**
     * @brief Process generic market event
     * @param event Market event (polymorphic)
     * @return true if successfully processed
     */
    bool process_event(const MarketEvent& event);
    
    /**
     * @brief Get event processing statistics
     */
    struct EventStats {
        uint64_t new_orders;
        uint64_t modifications;
        uint64_t deletions;
        uint64_t trades;
        uint64_t snapshots;
        uint64_t errors;
        
        EventStats() : new_orders(0), modifications(0), deletions(0),
                      trades(0), snapshots(0), errors(0) {}
    };
    
    const EventStats& get_stats() const { return stats_; }
    void reset_stats() { stats_ = EventStats(); }
    
    /**
     * @brief Get symbol for this handler
     */
    const std::string& get_symbol() const { return symbol_; }

private:
    std::string symbol_;
    OrderBook order_book_;
    EventStats stats_;
    
    /**
     * @brief Validate event before processing
     */
    bool validate_event(const MarketEvent& event) const;
};

} // namespace orderbook
