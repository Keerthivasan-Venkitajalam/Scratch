#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace orderbook {

/**
 * @brief Market data event types
 */
enum class EventType {
    NEW_ORDER,      // New order added to book
    MODIFY_ORDER,   // Existing order quantity modified
    DELETE_ORDER,   // Order removed from book
    TRADE,          // Trade execution
    SNAPSHOT        // Full book snapshot
};

/**
 * @brief Order side
 */
enum class Side {
    BID,  // Buy order
    ASK   // Sell order
};

/**
 * @brief Base market event structure
 */
struct MarketEvent {
    EventType type;
    uint64_t sequence_number;  // For ordering and gap detection
    uint64_t timestamp_ns;     // Event timestamp in nanoseconds
    std::string symbol;        // Instrument symbol
    
    MarketEvent(EventType t, uint64_t seq, uint64_t ts, std::string_view sym)
        : type(t)
        , sequence_number(seq)
        , timestamp_ns(ts)
        , symbol(sym) {}
    
    virtual ~MarketEvent() = default;
};

/**
 * @brief New order event
 * 
 * Represents a new order being added to the order book.
 */
struct NewOrderEvent : public MarketEvent {
    uint64_t order_id;
    Side side;
    double price;
    int64_t quantity;
    
    NewOrderEvent(uint64_t seq, uint64_t ts, std::string_view sym,
                  uint64_t oid, Side s, double p, int64_t q)
        : MarketEvent(EventType::NEW_ORDER, seq, ts, sym)
        , order_id(oid)
        , side(s)
        , price(p)
        , quantity(q) {}
};

/**
 * @brief Modify order event
 * 
 * Represents a change in order quantity (can be increase or decrease).
 */
struct ModifyOrderEvent : public MarketEvent {
    uint64_t order_id;
    Side side;
    double price;
    int64_t new_quantity;      // New total quantity
    int64_t quantity_delta;    // Change in quantity (can be negative)
    
    ModifyOrderEvent(uint64_t seq, uint64_t ts, std::string_view sym,
                     uint64_t oid, Side s, double p, int64_t new_qty, int64_t delta)
        : MarketEvent(EventType::MODIFY_ORDER, seq, ts, sym)
        , order_id(oid)
        , side(s)
        , price(p)
        , new_quantity(new_qty)
        , quantity_delta(delta) {}
};

/**
 * @brief Delete order event
 * 
 * Represents an order being removed from the book (cancel or fill).
 */
struct DeleteOrderEvent : public MarketEvent {
    uint64_t order_id;
    Side side;
    double price;
    int64_t quantity;  // Quantity being removed
    
    DeleteOrderEvent(uint64_t seq, uint64_t ts, std::string_view sym,
                     uint64_t oid, Side s, double p, int64_t q)
        : MarketEvent(EventType::DELETE_ORDER, seq, ts, sym)
        , order_id(oid)
        , side(s)
        , price(p)
        , quantity(q) {}
};

/**
 * @brief Trade event
 * 
 * Represents a trade execution (match between buy and sell orders).
 */
struct TradeEvent : public MarketEvent {
    uint64_t trade_id;
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    double price;
    int64_t quantity;
    Side aggressor_side;  // Which side initiated the trade
    
    TradeEvent(uint64_t seq, uint64_t ts, std::string_view sym,
               uint64_t tid, uint64_t buy_oid, uint64_t sell_oid,
               double p, int64_t q, Side aggressor)
        : MarketEvent(EventType::TRADE, seq, ts, sym)
        , trade_id(tid)
        , buy_order_id(buy_oid)
        , sell_order_id(sell_oid)
        , price(p)
        , quantity(q)
        , aggressor_side(aggressor) {}
};

/**
 * @brief Price level snapshot entry
 */
struct SnapshotLevel {
    double price;
    int64_t quantity;
    int32_t order_count;
    
    SnapshotLevel(double p, int64_t q, int32_t count)
        : price(p), quantity(q), order_count(count) {}
};

/**
 * @brief Snapshot event
 * 
 * Represents a full order book snapshot.
 * Used for initial book construction or recovery from gaps.
 */
struct SnapshotEvent : public MarketEvent {
    std::vector<SnapshotLevel> bids;  // Sorted descending by price
    std::vector<SnapshotLevel> asks;  // Sorted ascending by price
    
    SnapshotEvent(uint64_t seq, uint64_t ts, std::string_view sym)
        : MarketEvent(EventType::SNAPSHOT, seq, ts, sym) {}
    
    void add_bid(double price, int64_t quantity, int32_t order_count) {
        bids.emplace_back(price, quantity, order_count);
    }
    
    void add_ask(double price, int64_t quantity, int32_t order_count) {
        asks.emplace_back(price, quantity, order_count);
    }
};

/**
 * @brief Convert event type to string
 */
inline const char* event_type_to_string(EventType type) {
    switch (type) {
        case EventType::NEW_ORDER:    return "NEW_ORDER";
        case EventType::MODIFY_ORDER: return "MODIFY_ORDER";
        case EventType::DELETE_ORDER: return "DELETE_ORDER";
        case EventType::TRADE:        return "TRADE";
        case EventType::SNAPSHOT:     return "SNAPSHOT";
        default:                      return "UNKNOWN";
    }
}

/**
 * @brief Convert side to string
 */
inline const char* side_to_string(Side side) {
    return (side == Side::BID) ? "BID" : "ASK";
}

/**
 * @brief Convert side to char
 */
inline char side_to_char(Side side) {
    return (side == Side::BID) ? 'B' : 'A';
}

/**
 * @brief Convert char to side
 */
inline Side char_to_side(char c) {
    return (c == 'B' || c == 'b' || c == '1') ? Side::BID : Side::ASK;
}

} // namespace orderbook
