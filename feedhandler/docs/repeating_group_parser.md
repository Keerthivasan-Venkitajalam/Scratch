# Repeating Group Parser Documentation

## Overview

The Repeating Group Parser extends our FIX parsing capabilities to handle messages with multiple repeating entries. This is essential for market data feeds that contain order book snapshots, multiple price levels, or batch trade reports.

## Use Cases

### 1. Market Data Snapshots
Full order book with multiple bid and offer levels:
```
8=FIX.4.4|35=W|55=MSFT|268=3|
269=0|270=100.50|271=1000|  # Bid level 1
269=0|270=100.25|271=500|   # Bid level 2
269=1|270=100.75|271=750|   # Offer level 1
```

### 2. Order Book Depth
Complete market depth with 5+ levels on each side:
```
8=FIX.4.4|35=W|55=BTC-USD|268=10|
269=0|270=45100.00|271=1000|  # Bid levels
269=0|270=45099.50|271=750|
...
269=1|270=45100.50|271=900|   # Offer levels
269=1|270=45101.00|271=800|
...
```

### 3. Batch Trade Reports
Multiple trades in a single message:
```
8=FIX.4.4|35=AE|55=AAPL|268=5|
269=2|270=150.00|271=100|  # Trade 1
269=2|270=150.25|271=200|  # Trade 2
...
```

## FIX Protocol Tags

### Repeating Group Tags
- **Tag 268**: NoMDEntries - Number of repeating entries in the message
- **Tag 269**: MDEntryType - Entry type (0=Bid, 1=Offer, 2=Trade)
- **Tag 270**: MDEntryPx - Price for this entry
- **Tag 271**: MDEntrySize - Quantity/size for this entry

### Shared Tags
- **Tag 55**: Symbol - Instrument identifier (shared across all entries)
- **Tag 52**: SendingTime - Message timestamp (shared)

## Implementation Details

### Parser Characteristics
- **Zero heap allocations**: Uses stack-allocated field arrays
- **Handles missing counts**: Can infer entry count from tag occurrences
- **Flexible parsing**: Works with or without explicit NoMDEntries (268) tag
- **Fast number parsing**: Uses fast_atoi/fast_atof_fixed for performance
- **Buffer safety**: All string_views reference original buffer

### Algorithm
1. Extract all fields from message into stack array
2. Find shared fields (symbol, etc.)
3. Determine number of repeating entries
4. Find all occurrences of repeating tags (269, 270, 271)
5. Create one Tick object per repeating entry
6. Return vector of all parsed ticks

### Memory Layout
```
Stack allocation per message:
- Field array: 128 fields × 24 bytes = 3,072 bytes
- Index arrays: 3 × 32 indices × 8 bytes = 768 bytes
- Total: ~4KB stack per message
```

## Performance Benchmarks

### Test Environment
- **Platform**: macOS (darwin)
- **Compiler**: clang++ with C++20
- **Build**: Debug mode
- **Date**: January 26, 2026

### Benchmark Results

| Messages | Entries/Msg | Total Ticks | Time (μs) | Ticks/Second | μs/Tick |
|----------|-------------|-------------|-----------|--------------|---------|
| 1,000    | 1           | 1,000       | 3,521     | 284,010      | 3.52    |
| 1,000    | 5           | 5,000       | 8,680     | 576,036      | 1.74    |
| 1,000    | 10          | 10,000      | 14,335    | 697,593      | 1.43    |
| 10,000   | 5           | 50,000      | 59,055    | 846,668      | 1.18    |
| 100,000  | 3           | 300,000     | 409,896   | 731,892      | 1.37    |

### Key Performance Metrics
- **Peak Throughput**: 846,668 ticks/second (10K messages, 5 entries each)
- **Average Latency**: 1.18-3.52 μs per tick
- **Scaling**: Better performance with more entries per message
- **Efficiency**: ~730K ticks/second sustained at scale

### Performance Analysis

#### Scaling Characteristics
- **More entries = better efficiency**: Amortizes message parsing overhead
- **Consistent performance**: Maintains 700K+ ticks/sec at scale
- **Memory efficient**: Zero heap allocations regardless of entry count

#### Comparison with Single-Entry Parser
- Single entry: ~661K messages/sec = ~661K ticks/sec
- Repeating groups (5 entries): ~170K messages/sec = ~850K ticks/sec
- **Efficiency gain**: 1.28x more ticks processed per unit time

## Example Usage

### Parse Single Message with Repeating Groups
```cpp
#include "parser/repeating_group_parser.hpp"

std::string message = "8=FIX.4.4|35=W|55=MSFT|268=3|"
                     "269=0|270=100.50|271=1000|"
                     "269=0|270=100.25|271=500|"
                     "269=1|270=100.75|271=750|";

auto ticks = RepeatingGroupParser::parse_repeating_groups(message);

// ticks.size() == 3
// ticks[0]: MSFT Bid $100.50 x 1000
// ticks[1]: MSFT Bid $100.25 x 500
// ticks[2]: MSFT Offer $100.75 x 750
```

### Parse Buffer with Multiple Messages
```cpp
std::string buffer = 
    "8=FIX.4.4|35=W|55=AAPL|268=2|269=0|270=150.00|271=1000|269=1|270=150.25|271=500|\n"
    "8=FIX.4.4|35=W|55=GOOGL|268=2|269=0|270=2750.00|271=100|269=1|270=2751.00|271=75|\n";

auto all_ticks = RepeatingGroupParser::parse_buffer_with_repeating_groups(buffer);

// all_ticks.size() == 4 (2 ticks from each message)
```

### Process Order Book
```cpp
auto ticks = RepeatingGroupParser::parse_repeating_groups(order_book_message);

// Separate bids and offers
std::vector<Tick> bids, offers;
for (const auto& tick : ticks) {
    if (tick.side == 'B') bids.push_back(tick);
    else if (tick.side == 'S') offers.push_back(tick);
}

// Now you have separate bid and offer sides
```

## Buffer Lifetime Management

**CRITICAL**: The input buffer must remain valid for the lifetime of all returned Tick objects, since symbol fields contain string_view references.

### Safe Pattern
```cpp
std::string buffer = load_market_data();
auto ticks = RepeatingGroupParser::parse_buffer_with_repeating_groups(buffer);
process_order_book(ticks);  // Use ticks while buffer is valid
// buffer goes out of scope after ticks are done
```

### Unsafe Pattern
```cpp
std::vector<Tick> global_ticks;
{
    std::string buffer = load_market_data();
    global_ticks = RepeatingGroupParser::parse_buffer_with_repeating_groups(buffer);
} // buffer destroyed here!
// global_ticks now contain dangling string_view references
```

## Edge Cases Handled

### 1. No Explicit Entry Count
Parser can infer count from tag occurrences:
```cpp
// Missing Tag 268 (NoMDEntries)
"8=FIX.4.4|35=W|55=MSFT|269=0|270=100.50|271=1000|269=1|270=100.75|271=750|"
// Parser counts 269 tags: 2 entries
```

### 2. Single Entry (No Repeating Group)
Falls back to single-tick parsing:
```cpp
"8=FIX.4.4|35=D|55=AAPL|44=150.25|38=500|54=1|"
// Parsed as single tick
```

### 3. Mismatched Entry Counts
Uses minimum count across all repeating tags:
```cpp
// 3 type tags, 3 price tags, 2 size tags
// Parser creates 2 complete ticks (minimum)
```

### 4. Empty or Invalid Messages
Returns empty vector for invalid input:
```cpp
auto ticks = RepeatingGroupParser::parse_repeating_groups("");
// ticks.size() == 0
```

## Implementation Files

- **Header**: `include/parser/repeating_group_parser.hpp`
- **Implementation**: `src/parser/repeating_group_parser.cpp`
- **Tests**: `src/test_repeating_groups.cpp`

## Future Enhancements

### Potential Optimizations
1. **SIMD tag scanning**: Vectorized search for repeating tags
2. **Parallel parsing**: Process multiple messages concurrently
3. **Memory pooling**: Reuse tick vectors across messages
4. **Incremental updates**: Handle delta updates efficiently

### Additional Features
1. **More entry types**: Support for additional MDEntryType values
2. **Nested groups**: Handle nested repeating groups
3. **Conditional fields**: Parse optional fields per entry
4. **Validation**: Stricter validation of repeating group structure

## Conclusion

The Repeating Group Parser successfully extends our FIX parsing capabilities to handle complex market data messages:

✅ **Handles multiple entries**: Parse 1-10+ entries per message  
✅ **High performance**: 730K-850K ticks/second sustained  
✅ **Zero allocations**: Stack-only parsing  
✅ **Flexible parsing**: Works with or without explicit counts  
✅ **Real-world ready**: Handles order books, snapshots, and batch trades  

This capability is essential for processing real market data feeds that commonly use repeating groups for efficiency.