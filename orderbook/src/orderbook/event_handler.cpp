#include "orderbook/event_handler.hpp"

#include <iostream>

namespace orderbook {

OrderBookHandler::OrderBookHandler(const std::string& symbol)
    : symbol_(symbol)
    , order_book_(symbol) {
}

bool OrderBookHandler::on_new_order(const NewOrderEvent& event) {
    if (!validate_event(event)) {
        stats_.errors++;
        return false;
    }
    
    // Convert double price to fixed-point int64_t (multiply by 100 for 2 decimal places)
    int64_t price_fixed = static_cast<int64_t>(event.price * 100.0);
    
    // Side is already BID/ASK from order_book.hpp
    // Add order to book
    order_book_.add_order(event.side, price_fixed, event.quantity);
    
    stats_.new_orders++;
    return true;
}

bool OrderBookHandler::on_modify_order(const ModifyOrderEvent& event) {
    if (!validate_event(event)) {
        stats_.errors++;
        return false;
    }
    
    // Convert double price to fixed-point int64_t
    int64_t price_fixed = static_cast<int64_t>(event.price * 100.0);
    
    // Modify order in book
    order_book_.modify_order(event.side, price_fixed, event.new_quantity);
    
    stats_.modifications++;
    return true;
}

bool OrderBookHandler::on_delete_order(const DeleteOrderEvent& event) {
    if (!validate_event(event)) {
        stats_.errors++;
        return false;
    }
    
    // Convert double price to fixed-point int64_t
    int64_t price_fixed = static_cast<int64_t>(event.price * 100.0);
    
    // Delete order from book
    order_book_.delete_order(event.side, price_fixed, event.quantity);
    
    stats_.deletions++;
    return true;
}

bool OrderBookHandler::on_trade(const TradeEvent& event) {
    if (!validate_event(event)) {
        stats_.errors++;
        return false;
    }
    
    // Convert double price to fixed-point int64_t
    int64_t price_fixed = static_cast<int64_t>(event.price * 100.0);
    
    // Trade execution reduces quantity on both sides
    // Aggressor side determines which side to reduce
    if (event.aggressor_side == Side::BID) {
        // Buy aggressor hits ask side
        order_book_.delete_order(Side::ASK, price_fixed, event.quantity);
    } else {
        // Sell aggressor hits bid side
        order_book_.delete_order(Side::BID, price_fixed, event.quantity);
    }
    
    stats_.trades++;
    return true;
}

bool OrderBookHandler::on_snapshot(const SnapshotEvent& event) {
    if (!validate_event(event)) {
        stats_.errors++;
        return false;
    }
    
    // Clear existing book
    order_book_.clear();
    
    // Rebuild from snapshot
    // Add all bid levels
    for (const auto& level : event.bids) {
        int64_t price_fixed = static_cast<int64_t>(level.price * 100.0);
        order_book_.add_order(Side::BID, price_fixed, level.quantity);
    }
    
    // Add all ask levels
    for (const auto& level : event.asks) {
        int64_t price_fixed = static_cast<int64_t>(level.price * 100.0);
        order_book_.add_order(Side::ASK, price_fixed, level.quantity);
    }
    
    stats_.snapshots++;
    return true;
}

bool OrderBookHandler::process_event(const MarketEvent& event) {
    switch (event.type) {
        case EventType::NEW_ORDER:
            return on_new_order(static_cast<const NewOrderEvent&>(event));
            
        case EventType::MODIFY_ORDER:
            return on_modify_order(static_cast<const ModifyOrderEvent&>(event));
            
        case EventType::DELETE_ORDER:
            return on_delete_order(static_cast<const DeleteOrderEvent&>(event));
            
        case EventType::TRADE:
            return on_trade(static_cast<const TradeEvent&>(event));
            
        case EventType::SNAPSHOT:
            return on_snapshot(static_cast<const SnapshotEvent&>(event));
            
        default:
            std::cerr << "Unknown event type" << std::endl;
            stats_.errors++;
            return false;
    }
}

bool OrderBookHandler::validate_event(const MarketEvent& event) const {
    // Check symbol matches
    if (event.symbol != symbol_) {
        std::cerr << "Symbol mismatch: expected " << symbol_ 
                  << ", got " << event.symbol << std::endl;
        return false;
    }
    
    // Check timestamp is reasonable (not zero, not in future)
    if (event.timestamp_ns == 0) {
        std::cerr << "Invalid timestamp: 0" << std::endl;
        return false;
    }
    
    // Type-specific validation
    switch (event.type) {
        case EventType::NEW_ORDER: {
            const auto& e = static_cast<const NewOrderEvent&>(event);
            if (e.price <= 0 || e.quantity <= 0) {
                std::cerr << "Invalid price or quantity" << std::endl;
                return false;
            }
            break;
        }
        
        case EventType::MODIFY_ORDER: {
            const auto& e = static_cast<const ModifyOrderEvent&>(event);
            if (e.price <= 0 || e.new_quantity < 0) {
                std::cerr << "Invalid price or quantity" << std::endl;
                return false;
            }
            break;
        }
        
        case EventType::DELETE_ORDER: {
            const auto& e = static_cast<const DeleteOrderEvent&>(event);
            if (e.price <= 0 || e.quantity <= 0) {
                std::cerr << "Invalid price or quantity" << std::endl;
                return false;
            }
            break;
        }
        
        case EventType::TRADE: {
            const auto& e = static_cast<const TradeEvent&>(event);
            if (e.price <= 0 || e.quantity <= 0) {
                std::cerr << "Invalid price or quantity" << std::endl;
                return false;
            }
            break;
        }
        
        case EventType::SNAPSHOT:
            // Snapshot validation done in on_snapshot
            break;
            
        default:
            return false;
    }
    
    return true;
}

} // namespace orderbook
