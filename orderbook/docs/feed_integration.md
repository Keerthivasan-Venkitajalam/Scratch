# FeedHandler Integration with OrderBook

## Overview

This document describes the integration between the FeedHandler (Month 1) and OrderBook (Month 2), enabling real-time order book reconstruction from market data feeds.

## Architecture

```
FIX Feed → FeedHandler → Tick Stream → FeedIntegration → Market Events → OrderBookHandler → OrderBook
```

### Components

1. **FeedHandler**: Parses FIX protocol messages into Tick objects
2. **FeedIntegration**: Converts Ticks to Market Events
3. **OrderBookHandler**: Processes events and updates order book
4. **OrderBook**: Maintains sorted price levels

## Data Flow

### 1. Tick to Event Conversion

```cpp
Tick {
    symbol: "AAPL"
    price: 15000 (fixed-point, = $150.00)
    qty: 100
    side: 'B' (Buy)
    timestamp: 1000000
}
↓
NewOrderEvent {
    symbol: "AAPL"
    price: 150.00 (double)
    quantity: 100
    side: BID
    sequence: 1000000
    timestamp: 1000000
}
```

### 2. Event Processing

```cpp
NewOrderEvent → OrderBookHandler::on_new_order()
                ↓
                Convert price: double → int64_t fixed-point
                ↓
                OrderBook::add_order(BID, 15000, 100)
                ↓
                Update sorted bid levels
```

## Sequence Number Handling

### Purpose
- Detect message loss/gaps in feed
- Ensure book consistency
- Trigger snapshot recovery when needed

### Implementation

```cpp
bool OrderBookHandler::validate_sequence(uint64_t seq) {
    if (last_sequence_ == 0) {
        // First message or after snapshot
        last_sequence_ = seq;
        return true;
    }
    
    if (seq != last_sequence_ + 1) {
        // Gap detected!
        gap_stats_.gaps_detected++;
        gap_stats_.messages_dropped += (seq - last_sequence_ - 1);
        return false;
    }
    
    last_sequence_ = seq;
    return true;
}
```

### Gap Detection

When a gap is detected:
1. Log the gap (sequence numbers missing)
2. Increment gap statistics
3. Reject the message
4. **In production**: Request snapshot to resync

## Snapshot Handling

### When Snapshots Are Needed

1. **Initial book construction**: First connection to feed
2. **Gap recovery**: After detecting message loss
3. **Periodic refresh**: Prevent drift from accumulated errors
4. **Reconnection**: After network interruption

### Snapshot Processing

```cpp
SnapshotEvent {
    sequence: 5000
    bids: [
        {price: 150.00, qty: 500, orders: 5},
        {price: 149.50, qty: 300, orders: 3},
        ...
    ]
    asks: [
        {price: 150.50, qty: 400, orders: 4},
        {price: 151.00, qty: 200, orders: 2},
        ...
    ]
}
```

Processing steps:
1. **Clear existing book**: `order_book_.clear()`
2. **Reset sequence**: `last_sequence_ = snapshot.sequence`
3. **Rebuild bids**: Add all bid levels
4. **Rebuild asks**: Add all ask levels
5. **Resume incremental updates**: Process subsequent messages

## Incremental Updates

After snapshot, process incremental updates:

```cpp
// Sequence: 5001, 5002, 5003, ...
NewOrderEvent(seq=5001) → validate_sequence() → process
ModifyOrderEvent(seq=5002) → validate_sequence() → process
DeleteOrderEvent(seq=5003) → validate_sequence() → process
```

### Update Types

1. **NEW_ORDER**: Add new price level or increase quantity
2. **MODIFY_ORDER**: Change quantity at existing level
3. **DELETE_ORDER**: Remove quantity or entire level
4. **TRADE**: Reduce quantity on aggressor side

## Multi-Symbol Support

FeedIntegration manages multiple order books:

```cpp
std::unordered_map<std::string, std::unique_ptr<OrderBookHandler>> handlers_;
```

Each symbol has its own:
- OrderBook instance
- Sequence number tracking
- Gap statistics
- Event statistics

## Error Handling

### Validation Checks

1. **Symbol matching**: Event symbol matches handler symbol
2. **Price validation**: Price > 0
3. **Quantity validation**: Quantity > 0 (or >= 0 for modify)
4. **Sequence validation**: No gaps in sequence numbers
5. **Timestamp validation**: Timestamp != 0

### Error Recovery

```cpp
if (!validate_sequence(seq)) {
    // Gap detected
    gap_stats_.gaps_detected++;
    
    // In production:
    // 1. Log the gap
    // 2. Request snapshot
    // 3. Buffer subsequent messages
    // 4. Apply buffered messages after snapshot
    
    return false;
}
```

## Performance Considerations

### Conversion Overhead

- **Price conversion**: `double → int64_t` (multiply by 100)
- **Side conversion**: `char → enum` (simple comparison)
- **Symbol copy**: `string_view → string` (one allocation per symbol)

### Memory Management

- **Per-symbol overhead**: ~1KB (OrderBook + handler)
- **Per-level overhead**: ~32 bytes (PriceLevel struct)
- **Total for 100 symbols, 20 levels each**: ~100KB

### Latency

- **Tick → Event**: <100ns (conversion)
- **Event → Book update**: <100ns (add/modify/delete)
- **Total latency**: <200ns (tick to book update)

## Testing

### Unit Tests

```cpp
// Test tick conversion
Tick tick = create_sample_tick();
auto event = integration.tick_to_event(tick);
assert(event->type == EventType::NEW_ORDER);

// Test sequence validation
handler.validate_sequence(1);  // OK
handler.validate_sequence(2);  // OK
handler.validate_sequence(4);  // Gap! Returns false

// Test snapshot recovery
SnapshotEvent snapshot = create_snapshot();
handler.on_snapshot(snapshot);
assert(handler.get_last_sequence() == snapshot.sequence);
```

### Integration Tests

```cpp
// End-to-end test
FeedIntegration integration;

// Process ticks
for (const auto& tick : ticks) {
    integration.process_tick(tick);
}

// Verify book state
auto* book = integration.get_order_book("AAPL");
assert(book->get_best_bid().price == expected_bid);
assert(book->get_best_ask().price == expected_ask);
```

## Production Deployment

### Configuration

```cpp
struct FeedConfig {
    bool enable_gap_detection = true;
    bool auto_request_snapshot = true;
    uint64_t snapshot_interval_ms = 60000;  // 1 minute
    uint64_t max_gap_size = 100;  // Request snapshot if gap > 100
};
```

### Monitoring

Track these metrics:
- **Ticks processed**: Total ticks received
- **Events generated**: Total events created
- **Gaps detected**: Number of sequence gaps
- **Messages dropped**: Total messages lost
- **Snapshots received**: Snapshot count
- **Processing latency**: Tick → book update time

### Alerting

Alert on:
- Gap rate > 0.1% (indicates feed issues)
- Snapshot frequency > 1/minute (indicates instability)
- Processing latency > 1μs (indicates performance degradation)

## Future Enhancements

1. **Buffered recovery**: Buffer messages during gap, apply after snapshot
2. **Multiple feeds**: Merge data from primary + backup feeds
3. **Conflation**: Combine multiple updates at same price level
4. **Persistence**: Save snapshots to disk for fast restart
5. **Replay**: Replay historical data for backtesting

## References

- FeedHandler documentation: `feedhandler/docs/`
- OrderBook design: `orderbook/docs/orderbook_design.md`
- Market events: `orderbook/include/orderbook/market_event.hpp`
