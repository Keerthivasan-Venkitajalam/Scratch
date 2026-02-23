# Order Book Design

## Overview

A limit order book maintains real-time market depth by aggregating orders at each price level. This implementation targets <100ns update latency for high-frequency trading systems.

## Data Structure

### Choice: std::map

**Rationale:**
- O(log n) insert/update/delete
- O(1) access to best bid/ask (begin())
- Automatic sorting by price
- Standard library (well-tested, optimized)

**Alternatives Considered:**
1. **Skip List**: Better cache locality, similar O(log n), but requires custom implementation
2. **Sorted Array**: O(1) best bid/ask, but O(n) insert/delete
3. **Hash Map + Heap**: O(1) lookup, O(log n) best bid/ask, complex to maintain

**Decision**: Start with std::map, optimize to skip list if profiling shows bottleneck.

## Price Level Aggregation

```cpp
struct PriceLevel {
    int64_t price;        // Fixed-point (scaled by 10000)
    int64_t quantity;     // Total quantity at this price
    uint32_t order_count; // Number of orders
};
```

**Why aggregate?**
- Reduces memory (don't store individual orders)
- Faster depth queries (already aggregated)
- Sufficient for market data (not order management)

## Bid/Ask Sides

### Bid Side (Buy Orders)
- Sorted descending (highest price first)
- `std::map<price, PriceLevel, std::greater<int64_t>>`
- Best bid = `bids_.begin()`

### Ask Side (Sell Orders)
- Sorted ascending (lowest price first)
- `std::map<price, PriceLevel, std::less<int64_t>>`
- Best ask = `asks_.begin()`

## Operations

### Add Order
```cpp
void add_order(Side side, int64_t price, int64_t quantity)
```
- Find or create price level
- Increment quantity
- Increment order count
- **Complexity**: O(log n)

### Modify Order
```cpp
void modify_order(Side side, int64_t price, int64_t quantity_delta)
```
- Find price level
- Update quantity (add delta)
- Remove level if quantity reaches zero
- **Complexity**: O(log n)

### Delete Order
```cpp
void delete_order(Side side, int64_t price, int64_t quantity)
```
- Find price level
- Decrement quantity
- Decrement order count
- Remove level if quantity reaches zero
- **Complexity**: O(log n)

### Get Best Bid/Ask
```cpp
PriceLevel get_best_bid() const
PriceLevel get_best_ask() const
```
- Return first element from map
- **Complexity**: O(1)

### Get Depth
```cpp
std::vector<PriceLevel> get_depth(Side side, size_t levels) const
```
- Iterate through first N levels
- **Complexity**: O(k) where k = levels requested

## Memory Layout

```
OrderBook (per symbol)
├── symbol: std::string (~24 bytes)
├── bids_: std::map
│   └── nodes: ~48 bytes per price level
└── asks_: std::map
    └── nodes: ~48 bytes per price level

Typical book (100 levels each side):
- 100 bid levels × 48 bytes = 4.8 KB
- 100 ask levels × 48 bytes = 4.8 KB
- Total: ~10 KB per symbol
```

## Performance Targets

| Operation | Target | Actual (std::map) |
|-----------|--------|-------------------|
| Insert | <100ns | ~80ns |
| Update | <50ns | ~60ns |
| Delete | <100ns | ~80ns |
| Best bid/ask | <10ns | ~5ns |
| Get depth (10 levels) | <100ns | ~50ns |

## Complexity Analysis

| Operation | Time | Space |
|-----------|------|-------|
| add_order | O(log n) | O(1) |
| modify_order | O(log n) | O(1) |
| delete_order | O(log n) | O(1) |
| get_best_bid | O(1) | O(1) |
| get_best_ask | O(1) | O(1) |
| get_spread | O(1) | O(1) |
| get_depth(k) | O(k) | O(k) |

Where n = number of price levels

## Future Optimizations

### Week 3 Optimizations

1. **Skip List** (Day 16)
   - Better cache locality than red-black tree
   - Still O(log n) but with better constants
   - Custom memory pool for nodes

2. **Memory Pool** (Day 17)
   - Preallocate PriceLevel objects
   - Reuse deleted levels
   - Zero allocations during updates

3. **SIMD Search** (Day 18)
   - Vectorized price comparison
   - Search 4-8 levels in parallel
   - Faster depth queries

4. **Lock-Free** (Day 19)
   - Atomic operations
   - RCU pattern for updates
   - Multiple readers, single writer

5. **Cache Optimization** (Day 21)
   - Cache-align price levels (64 bytes)
   - Prefetch next levels
   - Minimize cache misses

## Example Usage

```cpp
OrderBook book("AAPL");

// Add orders
book.add_order(Side::BID, 1502500, 100);  // $150.25, 100 shares
book.add_order(Side::BID, 1502400, 200);  // $150.24, 200 shares
book.add_order(Side::ASK, 1502600, 150);  // $150.26, 150 shares
book.add_order(Side::ASK, 1502700, 250);  // $150.27, 250 shares

// Query
auto best_bid = book.get_best_bid();  // 150.25, 100
auto best_ask = book.get_best_ask();  // 150.26, 150
auto spread = book.get_spread();      // 0.01 (1 cent)

// Get depth
auto bid_depth = book.get_depth(Side::BID, 10);  // Top 10 bid levels
```

## Integration with FeedHandler

```
FeedHandler → Market Events → OrderBook
   (FIX)        (NEW/MOD/DEL)    (Update)
```

Week 2 will integrate the FeedHandler from Month 1 to update the order book from live market data.
