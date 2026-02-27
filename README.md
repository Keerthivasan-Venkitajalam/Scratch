# Quant Finance Systems - C++ Trading Infrastructure

A high-performance C++ trading infrastructure built from scratch, including market data processing and order book reconstruction for high-frequency trading systems.

## Overview

This repository contains a complete trading system stack built over 2 months:

- **Month 1**: FeedHandler - Zero-copy FIX protocol parser with <1μs latency
- **Month 2**: OrderBook - Real-time limit order book with <100ns update latency

## Projects

### 1. FeedHandler (Month 1) - COMPLETE

High-performance market data parser for FIX protocol messages.

**Key Features:**
- Zero-copy parsing with string_view
- Finite State Machine for streaming TCP data
- Handles fragmented messages seamlessly
- 2.46M messages/second throughput
- Garbage recovery for corrupted data
- Multi-threaded architecture

**Performance:**
- Naive parser: 376k msg/s
- StringView parser: 2.46M msg/s (6.9× faster)
- FSM parser: 1.18M msg/s + streaming support

[See FeedHandler Documentation →](feedhandler/docs/)

### 2. OrderBook (Month 2) - IN PROGRESS

Real-time limit order book reconstruction from market data.

**Key Features:**
- Price-level aggregation
- O(log n) insert/update/delete operations
- O(1) best bid/ask queries
- O(k) market depth queries
- Multi-symbol support (planned)

**Status:** Week 1 Complete - Foundation implemented
- Day 1-4: Order book operations (add, modify, delete, query)
- Day 5: Market depth query with comprehensive tests
- Day 6: Algorithm sprint (LRU Cache, Find Median)

[See OrderBook Documentation →](orderbook/docs/)

## Building

### FeedHandler

```bash
cd feedhandler
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8

# Run tests
./build/test_fsm_parser
./build/test_streaming_handler

# Run benchmarks
./build/gbench_parsers
```

### OrderBook

```bash
cd orderbook
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8

# Run tests
./build/price_level_tests
```

## Architecture

```
.
├── feedhandler/          # Month 1: Market data parser
│   ├── include/
│   │   ├── common/       # Tick, pools, flyweight
│   │   ├── net/          # TCP client, buffers
│   │   ├── parser/       # FIX parsers (naive, FSM, streaming)
│   │   └── threading/    # Multi-threaded feedhandler
│   ├── src/
│   ├── tests/            # GTest unit tests
│   ├── benchmarks/       # Google Benchmark suite
│   └── docs/             # Technical documentation
│
├── orderbook/            # Month 2: Order book reconstruction
│   ├── include/
│   │   └── orderbook/    # OrderBook, PriceLevel
│   ├── src/
│   ├── tests/            # GTest unit tests
│   └── docs/             # Design documentation
│
└── algorithms/           # LeetCode/Codeforces solutions
```

## Performance Targets

### FeedHandler - ACHIEVED
- ACHIEVED: 1M+ messages/second single core (achieved 2.46M)
- ACHIEVED: Zero allocations in hot path
- ACHIEVED: Handles TCP fragmentation
- ACHIEVED: Garbage recovery from corruption

### OrderBook - TARGET
- <100ns update latency
- <10ns best bid/ask query
- 1M updates/second
- 100+ concurrent symbols

## Technology Stack

- **Language**: C++20
- **Build**: CMake 3.15+
- **Testing**: Google Test
- **Benchmarking**: Google Benchmark
- **Compiler**: Clang/GCC with -O3 optimization

## Key Optimizations

### FeedHandler
1. Zero-copy parsing with `std::string_view`
2. Custom `fast_atoi`/`fast_atof` (no exceptions)
3. Branch prediction hints (`__builtin_expect`)
4. Object pooling for Tick allocation
5. Flyweight pattern for memory efficiency
6. Lock-free message queue for threading

### OrderBook
1. `std::map` for O(log n) operations
2. Price-level aggregation (not individual orders)
3. Fixed-point arithmetic (no floating point)
4. Future: Skip list, SIMD, lock-free updates

## Documentation

### FeedHandler
- [Month 1 Completion Summary](feedhandler/docs/month1_completion_summary.md)
- [FSM Parser Implementation](feedhandler/docs/fsm_parser_implementation.md)
- [Google Benchmark Report](feedhandler/docs/google_benchmark_report.md)
- [Garbage Recovery](feedhandler/docs/garbage_recovery.md)
- [Threading Architecture](feedhandler/docs/threading_architecture.md)

### OrderBook
- [Order Book Design](orderbook/docs/orderbook_design.md)

## Testing

### FeedHandler Tests
```bash
cd feedhandler/build

# Unit tests
./test_fsm_parser
./test_streaming_handler
./test_fast_number_parser
./test_repeating_groups
./test_garbage_recovery
./test_tick_pool
./test_threaded_feedhandler

# Benchmarks
./gbench_parsers
./parser_benchmark
```

### OrderBook Tests
```bash
cd orderbook/build

# Unit tests
./price_level_tests
./order_book_tests

# Market depth demo
./test_market_depth
```

## Algorithm Solutions

The `algorithms/` directory contains LeetCode and Codeforces solutions:

- LeetCode 344: Reverse String
- LeetCode 151: Reverse Words (in-place)
- LeetCode 8: String to Integer (atoi)
- LeetCode 65: Valid Number (FSM)
- LeetCode 10: Regular Expression Matching (DP)
- LeetCode 146: LRU Cache (hash map + doubly linked list)
- LeetCode 295: Find Median from Data Stream (two heaps)
- LeetCode 3: Longest Substring Without Repeating Characters
- Codeforces: String Task

All solutions include complexity analysis and test cases.

## Requirements

- C++20 compatible compiler (Clang 12+ or GCC 10+)
- CMake 3.15 or higher
- POSIX-compliant system (Linux, macOS)
- Python 3 (for test discovery)

## Learning Path

This project follows a structured 4-month learning plan:

1. **Month 1**: Market data infrastructure (FeedHandler) - COMPLETE
2. **Month 2**: Order book reconstruction - IN PROGRESS (Week 1 complete)
3. **Month 3**: Strategy backtesting engine
4. **Month 4**: Risk management & portfolio optimization

Each month builds on the previous, creating a complete trading system from scratch.

## Performance Benchmarks

### FeedHandler Parsers

| Parser | Latency | Throughput | Speedup |
|--------|---------|------------|---------|
| Naive | 2,985 ns | 376k msg/s | 1.0× |
| StringView | 430 ns | 2.46M msg/s | 6.9× |
| FSM | 924 ns | 1.18M msg/s | 3.2× |

### OrderBook Operations (Target)

| Operation | Target | Complexity |
|-----------|--------|------------|
| Insert | <100ns | O(log n) |
| Update | <50ns | O(log n) |
| Delete | <100ns | O(log n) |
| Best bid/ask | <10ns | O(1) |
| Get depth | <100ns | O(k) |

## License

Educational project - built for learning low-latency systems programming.

## Contact

Built as part of a structured quant finance learning curriculum.
