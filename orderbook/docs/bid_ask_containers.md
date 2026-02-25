# Bid/Ask Container Implementation

## Overview

The OrderBook uses `std::map` with custom comparators to maintain sorted price levels for bid and ask sides. This design ensures O(log n) operations while keeping the best bid/ask accessible in O(1) time.

## Container Choice: std::map

### Why std::map?

1. **Automatic Sorting**: Maintains price levels in sorted order
2. **Efficient Operations**: O(log n) insert/update/delete
3. **Fast Best Price Access**: O(1) via `begin()`
4. **Standard Library**: Well-tested, optimized implementation
5. **Cache-Friendly**: Red-black tree with good locality

### Alternatives Considered

| Data Structure | Insert | Delete | Best Price | Depth Query | Notes |
|----------------|--------|--------|------------|-------------|-------|
| **std::map** | O(log n) | O(log n) | O(1) | O(k) | ✓ Chosen |
| std::vector (sorted) | O(n) | O(n) | O(1) | O(k) | Too slow for updates |
| std::priority_queue | O(log n) | O(n) | O(1) | O(n) | Can't delete efficiently |
| Skip List | O(log n) | O(log n) | O(1) | O(k) | Future optimization |
| Array (fixed prices) | O(1) | O(1) | O(n) | O(k) | Only for tick-based |

## Bid Side: Descending Order

```cpp
std::map<int64_t, PriceLevel, std::greater<int64_t>> bids_;
```

### Comparator: std::greater

- **Highest price first**: Best bid is at `begin()`
- **Natural iteration**: Iterating gives prices from high to low
- **Depth query**: First k elements are top k levels

### Example

```
Bids (descending):
150.50 → 100 shares
150.00 → 200 shares  
149.50 → 150 shares
149.00 → 300 shares

bids_.begin() → 150.50 (best bid)
```

## Ask Side: Ascending Order

```cpp
std::map<int64_t, PriceLevel, std::less<int64_t>> asks_;
```

### Comparator: std::less (default)

- **Lowest price first**: Best ask is at `begin()`
- **Natural iteration**: Iterating gives prices from low to high
- **Depth query**: First k elements are top k levels

### Example

```
Asks (ascending):
150.50 → 100 shares
151.00 → 200 shares
151.50 → 150 shares
152.00 → 300 shares

asks_.begin() → 150.50 (best ask)
```

## Operations

### Add Order

```cpp
void add_order(Side side, int64_t price, int64_t quantity);
```

**Algorithm**:
1. Find price level in map
2. If exists: aggregate quantity, increment order count
3. If new: create price level entry
4. Complexity: O(log n)

**Example**:
```cpp
book.add_order(Side::BID, to_fixed(150.00), 100);
book.add_order(Side::BID, to_fixed(150.00), 50);  // Aggregates to 150
```

### Modify Order

```cpp
void modify_order(Side side, int64_t price, int64_t quantity_delta);
```

**Algorithm**:
1. Find price level
2. Apply quantity delta (can be positive or negative)
3. If quantity ≤ 0: remove price level
4. Complexity: O(log n)

**Example**:
```cpp
book.modify_order(Side::BID, to_fixed(150.00), -30);  // Reduce by 30
```

### Delete Order

```cpp
void delete_order(Side side, int64_t price, int64_t quantity);
```

**Algorithm**:
1. Find price level
2. Subtract quantity
3. Decrement order count
4. If quantity ≤ 0: remove price level
5. Complexity: O(log n)

**Example**:
```cpp
book.delete_order(Side::BID, to_fixed(150.00), 100);
```

### Get Best Bid/Ask

```cpp
PriceLevel get_best_bid() const;
PriceLevel get_best_ask() const;
```

**Algorithm**:
1. Return `begin()->second` (first element)
2. Complexity: O(1)

**Why O(1)?**
- `std::map::begin()` is constant time
- Custom comparators ensure best price is always first

### Get Depth

```cpp
std::vector<PriceLevel> get_depth(Side side, size_t levels) const;
```

**Algorithm**:
1. Iterate from `begin()` for k levels
2. Copy price levels to vector
3. Complexity: O(k) where k = requested levels

**Example**:
```cpp
auto depth = book.get_depth(Side::BID, 5);  // Top 5 bid levels
```

## Sorting Guarantees

### Bid Side Invariant

```
∀ i, j: i < j ⇒ bids[i].price > bids[j].price
```

The i-th bid always has a higher price than the (i+1)-th bid.

### Ask Side Invariant

```
∀ i, j: i < j ⇒ asks[i].price < asks[j].price
```

The i-th ask always has a lower price than the (i+1)-th ask.

## Test Coverage

### Sorting Tests

✓ Bids sorted descending (highest first)
✓ Asks sorted ascending (lowest first)
✓ Best bid is highest price
✓ Best ask is lowest price

### Operation Tests

✓ Add single order
✓ Add multiple orders at same price (aggregation)
✓ Modify order (increase/decrease quantity)
✓ Delete partial quantity
✓ Delete entire quantity (removes level)

### Edge Cases

✓ Empty book returns invalid spread
✓ Zero quantity ignored
✓ Negative quantity ignored
✓ Delete non-existent price does nothing

### Complex Scenarios

✓ Realistic market with multiple levels
✓ Spread calculation
✓ Mid price calculation
✓ Depth queries with limits

## Performance Characteristics

### Time Complexity

| Operation | Complexity | Notes |
|-----------|------------|-------|
| add_order | O(log n) | Map insertion/update |
| modify_order | O(log n) | Map lookup + update |
| delete_order | O(log n) | Map lookup + erase |
| get_best_bid | O(1) | First element |
| get_best_ask | O(1) | First element |
| get_spread | O(1) | Two O(1) lookups |
| get_depth(k) | O(k) | Iterate k levels |
| clear | O(n) | Clear both maps |

### Space Complexity

- **Per price level**: ~64 bytes (map node + PriceLevel)
- **Total**: O(n) where n = number of price levels
- **Typical book**: 20-50 levels per side = ~5KB

### Cache Performance

- **Red-black tree**: Good cache locality for small books
- **Sequential access**: Depth queries benefit from prefetching
- **Hot path**: Best bid/ask in L1 cache

## Future Optimizations

### 1. Skip List (Week 3 Day 16)

Replace `std::map` with custom skip list:
- Better cache locality
- Faster iteration
- Lock-free variants possible

### 2. Memory Pool (Week 3 Day 17)

Preallocate price level nodes:
- Zero allocations during updates
- Reuse deleted nodes
- Predictable latency

### 3. SIMD Search (Week 3 Day 18)

Vectorized price comparison:
- Search multiple levels in parallel
- AVX2 instructions
- Faster depth queries

### 4. Lock-Free Updates (Week 3 Day 19)

Atomic operations:
- RCU (Read-Copy-Update) pattern
- Multiple readers, single writer
- No mutex overhead

## Usage Example

```cpp
#include "orderbook/order_book.hpp"

using namespace orderbook;

int main() {
    OrderBook book("AAPL");
    
    // Build order book
    book.add_order(Side::BID, to_fixed(150.00), 500);
    book.add_order(Side::BID, to_fixed(149.95), 300);
    book.add_order(Side::ASK, to_fixed(150.05), 400);
    book.add_order(Side::ASK, to_fixed(150.10), 600);
    
    // Query best prices
    auto best_bid = book.get_best_bid();
    auto best_ask = book.get_best_ask();
    
    std::cout << "Best Bid: " << to_double(best_bid.price) 
              << " x " << best_bid.quantity << std::endl;
    std::cout << "Best Ask: " << to_double(best_ask.price) 
              << " x " << best_ask.quantity << std::endl;
    
    // Get market depth
    auto bid_depth = book.get_depth(Side::BID, 5);
    std::cout << "\nBid Depth:" << std::endl;
    for (const auto& level : bid_depth) {
        std::cout << "  " << to_double(level.price) 
                  << " x " << level.quantity << std::endl;
    }
    
    return 0;
}
```

## Conclusion

The bid/ask container implementation using `std::map` with custom comparators provides:

- **Correct sorting**: Bids descending, asks ascending
- **Efficient operations**: O(log n) updates, O(1) best price
- **Simple API**: Easy to use and understand
- **Production-ready**: Comprehensive test coverage
- **Optimizable**: Clear path to future improvements

This foundation supports the real-time order book reconstruction required for high-frequency trading systems.
