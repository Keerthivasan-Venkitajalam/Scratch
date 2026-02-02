# Naive FIX Parser Benchmark Results

## Overview

This document records the performance baseline for our naive FIX parser implementation using standard library components (`std::string`, `std::stringstream`, `std::getline`).

## Implementation Details

### Parser Characteristics
- **Approach**: String-based parsing with heap allocations
- **Components**: `std::stringstream` + `std::getline('|')` + `std::map<int, std::string>`
- **Memory**: Multiple heap allocations per message
- **Thread Safety**: Not thread-safe (uses static storage for symbols)

### Parsed Fields
- **Tag 55**: Symbol (instrument identifier)
- **Tag 44**: Price (converted to fixed-point int64_t)
- **Tag 38**: OrderQty (quantity/size)
- **Tag 54**: Side (1=Buy→'B', 2=Sell→'S')

## Benchmark Results

### Test Environment
- **Platform**: macOS (darwin)
- **Compiler**: clang++ with C++20
- **Build**: Debug mode
- **Date**: January 26, 2026

### Performance Metrics

| Messages | Total Time (μs) | Messages/Second | μs/Message |
|----------|----------------|-----------------|------------|
| 1,000    | 13,492         | 74,117          | 13.49      |
| 10,000   | 127,148        | 78,648          | 12.71      |
| 100,000  | 915,065        | 109,281         | 9.15       |
| 1,000,000| 8,716,029      | 114,731         | 8.72       |

### Key Observations

1. **Throughput**: ~115K messages/second at scale
2. **Latency**: ~8.7 μs per message for large batches
3. **Scaling**: Performance improves with larger batch sizes (better cache utilization)
4. **Memory**: Heavy heap allocation overhead from string operations

## Sample Test Data

### Input Message
```
8=FIX.4.4|9=79|35=D|55=MSFT|44=123.4500|38=1000|54=1|52=20240131-12:34:56|10=020|
```

### Parsed Output
```
Symbol: MSFT
Price: $123.4500
Quantity: 1000
Side: B (Buy)
Valid: Yes
```

## Performance Analysis

### Bottlenecks Identified
1. **String Allocations**: Each field extraction creates new strings
2. **Stream Overhead**: `std::stringstream` has parsing overhead
3. **Map Lookups**: `std::map<int, std::string>` for field storage
4. **Exception Handling**: `std::stoi`/`std::stod` exception overhead

### Memory Usage Pattern
- ~200+ bytes allocated per message (estimated)
- Frequent allocation/deallocation cycles
- Poor cache locality due to scattered allocations

## Comparison Target

This naive implementation serves as the baseline for optimization. Our goals for improved parsers:

- **Week 2 Day 2**: `string_view` parser (target: 2-3x faster)
- **Week 3**: FSM parser (target: 5x faster, >500K messages/second)
- **Week 4**: Optimized FSM (target: >1M messages/second)

## Implementation Files

- **Header**: `include/parser/naive_fix_parser.hpp`
- **Implementation**: `src/parser/naive_fix_parser.cpp`
- **Benchmark**: `src/parser_benchmark.cpp`

## Next Steps

1. Implement `string_view`-based parser to eliminate string allocations
2. Replace `std::stringstream` with direct character parsing
3. Use custom `fast_atoi`/`fast_atof` functions
4. Implement finite state machine for streaming parse capability