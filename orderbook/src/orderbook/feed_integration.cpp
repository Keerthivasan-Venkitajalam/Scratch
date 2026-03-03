#include "orderbook/feed_integration.hpp"

#include <iostream>

namespace orderbook {

FeedIntegration::FeedIntegration() {
}

bool FeedIntegration::process_tick(const feedhandler::common::Tick& tick) {
    stats_.ticks_processed++;
    
    // Get symbol from tick
    std::string symbol(tick.symbol.data(), tick.symbol.size());
    
    // Get or create handler for this symbol
    auto& handler = get_handler(symbol);
    
    // Convert tick to market event
    auto event = tick_to_event(tick);
    if (!event) {
        stats_.errors++;
        return false;
    }
    
    // Process event through handler
    bool success = handler.process_event(*event);
    if (success) {
        stats_.events_generated++;
    } else {
        stats_.errors++;
    }
    
    return success;
}

OrderBookHandler& FeedIntegration::get_handler(const std::string& symbol) {
    auto it = handlers_.find(symbol);
    if (it == handlers_.end()) {
        // Create new handler for this symbol
        auto handler = std::make_unique<OrderBookHandler>(symbol);
        auto& ref = *handler;
        handlers_[symbol] = std::move(handler);
        return ref;
    }
    return *it->second;
}

OrderBook* FeedIntegration::get_order_book(const std::string& symbol) {
    auto it = handlers_.find(symbol);
    if (it == handlers_.end()) {
        return nullptr;
    }
    return &it->second->get_order_book();
}

std::vector<std::string> FeedIntegration::get_symbols() const {
    std::vector<std::string> symbols;
    symbols.reserve(handlers_.size());
    
    for (const auto& pair : handlers_) {
        symbols.push_back(pair.first);
    }
    
    return symbols;
}

std::unique_ptr<MarketEvent> FeedIntegration::tick_to_event(const feedhandler::common::Tick& tick) {
    // Extract symbol
    std::string symbol(tick.symbol.data(), tick.symbol.size());
    
    // Convert price from fixed-point to double
    double price = feedhandler::common::price_to_double(tick.price);
    
    // Determine side from tick.side ('B' or 'S')
    Side side = (tick.side == 'B') ? Side::BID : Side::ASK;
    
    // Generate sequence number from timestamp
    uint64_t seq = tick.timestamp;
    
    // Create NewOrderEvent (simplified - in real system would parse order type)
    // Using timestamp as order_id for simplicity
    auto event = std::make_unique<NewOrderEvent>(
        seq,                    // sequence number
        tick.timestamp,         // timestamp
        symbol,                 // symbol
        tick.timestamp,         // order_id (using timestamp as proxy)
        side,                   // side
        price,                  // price
        tick.qty                // quantity
    );
    
    return event;
}

} // namespace orderbook
