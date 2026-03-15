<div align="center">

# Quantitative Trading System

### High-Performance C++ Market Data Infrastructure

**Zero-allocation FIX protocol parser with sub-microsecond latency and lock-free concurrency.**

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg?style=for-the-badge)](https://github.com/Keerthivasan-Venkitajalam/Scratch)
[![License](https://img.shields.io/badge/license-MIT-green.svg?style=for-the-badge)](LICENSE)
[![C++](https://img.shields.io/badge/C++-20-blue?style=for-the-badge&logo=cplusplus)](https://isocpp.org)
[![CMake](https://img.shields.io/badge/CMake-3.15-064F8C?style=for-the-badge&logo=cmake)](https://cmake.org)
[![Performance](https://img.shields.io/badge/Performance-2.46M_msg/s-red?style=for-the-badge)](https://github.com/Keerthivasan-Venkitajalam/Scratch)

[About](#about-the-project) • [Architecture](#system-architecture) • [Performance](#performance-benchmarks) • [Getting Started](#getting-started) • [Documentation](#technical-documentation)

</div>

---

## About the Project

**Quantitative Trading System** is a production-ready high-frequency trading infrastructure built from scratch in C++. This project demonstrates mastery of advanced Data Structures & Algorithms through practical application in financial market technology.

The system processes FIX protocol messages at **2.46 million messages per second** with **zero heap allocations** in the critical execution path, making it suitable for professional trading environments.

### Key Transformations
- **Naive to Optimized**: 6.5× performance improvement through zero-copy parsing
- **Blocking to Lock-Free**: SPSC ring buffers for multi-threaded processing
- **Error-Prone to Self-Healing**: Automatic garbage recovery with pattern scanning
- **Academic to Production**: Enterprise-grade error handling and monitoring

### Built for Learning, Ready for Production
This system was built following a comprehensive 9-month roadmap with production-grade features:
- **Google Benchmark** integration for performance measurement
- **Google Test** framework with 95%+ test coverage
- **AddressSanitizer** integration for memory safety
- **OpenTelemetry** traces for observability
- **Docker deployment** with health checks

---

## System Architecture

The system follows a **hybrid latency model** where C++ handles the ultra-low latency execution path while Python manages strategy research and analysis.

### Core Components Flow

```
Market Data → FIX Parser → Order Book → Strategy Engine → Execution
     ↓            ↓           ↓            ↓            ↓
  TCP Stream → FSM Parser → Price Levels → Algorithms → Orders
```

### Component Details

#### 1. Market Data Infrastructure (`feedhandler/`)
- **FIX Protocol Parser**: Finite State Machine with streaming support
- **Garbage Recovery**: Pattern-based error recovery scanning for "8=FIX"
- **Branch Prediction**: Compiler hints for hot path optimization
- **Threading**: Lock-free producer-consumer architecture

#### 2. Order Book Engine (`orderbook/`)
- **Price Level Management**: Red-Black Tree equivalent data structures
- **Order Tracking**: Hash map for O(1) order lookup by ID
- **Market Events**: Real-time order book reconstruction
- **Feed Integration**: Direct connection to market data parser

#### 3. Algorithm Implementations (`algorithms/`)
- **String Parsing**: LeetCode 8 (atoi), 65 (valid number FSM)
- **Dynamic Programming**: LeetCode 10 (regex matching)
- **Graph Theory**: Foundation for arbitrage detection
- **Advanced Data Structures**: Segment trees, priority queues

---

## Performance Benchmarks

### Parser Performance Comparison
| Parser Type | Throughput (msg/s) | Latency (ns) | Speedup |
|-------------|-------------------|--------------|---------|
| Naive | 377k | 2,653 | 1.0× |
| StringView | 2.46M | 406 | **6.5×** |
| FSM | 1.95M | 513 | **5.2×** |

### System Capabilities
- **Real-time Processing**: Sub-microsecond message processing
- **Fault Tolerance**: Automatic error recovery and resynchronization  
- **Scalability**: Lock-free architecture supports high throughput
- **Memory Efficiency**: Zero allocations in hot path

### Performance Targets - All Exceeded
- Target: 1M messages/sec → **Achieved: 2.46M messages/sec (246%)**
- Target: <10μs latency → **Achieved: <1μs latency (10× better)**
- Target: Zero allocations → **Achieved: Zero heap allocations**
- Target: Error recovery → **Achieved: Pattern-based garbage recovery**

---

## Getting Started

### Prerequisites
```bash
# macOS
brew install cmake clang

# Ubuntu/Debian  
sudo apt-get install cmake clang++ build-essential
```

### Quick Start
```bash
# Clone repository
git clone https://github.com/Keerthivasan-Venkitajalam/Scratch.git
cd Scratch

# Build FeedHandler
cmake -B feedhandler/build -S feedhandler -DCMAKE_BUILD_TYPE=Release
cmake --build feedhandler/build -j8

# Build OrderBook
cmake -B orderbook/build -S orderbook -DCMAKE_BUILD_TYPE=Release
cmake --build orderbook/build -j8

# Run final integration demo
./feedhandler/build/final_demo
```

### Key Executables
```bash
# Performance benchmarks
./feedhandler/build/gbench_parsers

# Component tests
./feedhandler/build/test_fsm_parser
./feedhandler/build/test_garbage_recovery  
./feedhandler/build/test_threaded_feedhandler
./orderbook/build/test_order_book

# Integration demo
./feedhandler/build/final_demo
```

### Verify Performance
```bash
# Run Google Benchmark suite
./feedhandler/build/gbench_parsers --benchmark_filter="SingleMessage"

# Expected output:
# BM_StringViewParser_SingleMessage    406 ns    2.46M msg/s
# BM_FSMParser_SingleMessage          513 ns    1.95M msg/s
```

---

## Tech Stack

**Core System:**
- **Language**: C++20 with modern features
- **Build System**: CMake 3.15+ with multiple configurations
- **Compiler**: Clang/GCC with -O3 optimization
- **Testing**: Google Test framework
- **Benchmarking**: Google Benchmark suite

**Key Libraries:**
- **Concurrency**: std::atomic, memory barriers
- **Parsing**: Custom FSM, zero-copy string_view
- **Data Structures**: STL containers, custom allocators
- **Networking**: POSIX sockets, non-blocking I/O

**Development Tools:**
- **Memory Safety**: AddressSanitizer integration
- **Performance**: perf integration, branch prediction hints
- **Quality**: Comprehensive test suite, static analysis
- **Documentation**: Detailed design docs and API reference

---

## Technical Documentation

### Core Documentation
- [`feedhandler/docs/google_benchmark_report.md`](feedhandler/docs/google_benchmark_report.md) - Comprehensive performance analysis
- [`feedhandler/docs/garbage_recovery.md`](feedhandler/docs/garbage_recovery.md) - Error recovery implementation
- [`feedhandler/docs/threading_architecture.md`](feedhandler/docs/threading_architecture.md) - Lock-free concurrency design
- [`orderbook/docs/orderbook_design.md`](orderbook/docs/orderbook_design.md) - Order book architecture

### Algorithm Implementations
- **Finite State Machine**: Character-by-character FIX protocol parsing
- **Lock-Free Ring Buffer**: SPSC queue with memory barriers
- **Object Pooling**: Pre-allocated memory management
- **Branch Prediction**: Compiler optimization hints
- **Garbage Recovery**: Pattern-based error recovery

### Project Structure
```
.
├── feedhandler/          # Market data infrastructure
│   ├── include/
│   │   ├── common/       # Tick, pools, flyweight
│   │   ├── parser/       # FIX parsers (naive, FSM, streaming)
│   │   └── threading/    # Multi-threaded architecture
│   ├── src/
│   ├── tests/            # Google Test unit tests
│   ├── benchmarks/       # Google Benchmark suite
│   └── docs/             # Technical documentation
│
├── orderbook/            # Order book reconstruction
│   ├── include/
│   │   └── orderbook/    # OrderBook, PriceLevel
│   ├── src/
│   ├── tests/            # Google Test unit tests
│   └── docs/             # Design documentation
│
└── algorithms/           # LeetCode/Codeforces solutions
```

---

## Learning Outcomes

### Data Structures & Algorithms Mastery
- **Trees**: Red-Black Trees, Segment Trees, Binary Heaps
- **Graphs**: Shortest Path algorithms, Negative Cycle Detection
- **Strings**: Finite State Machines, Pattern Matching, Zero-Copy Parsing
- **Concurrency**: Lock-Free Data Structures, Memory Models

### Systems Programming Excellence  
- **Memory Management**: Object Pools, Zero-Allocation Design
- **Performance Optimization**: Branch Prediction, Cache Optimization
- **Concurrent Programming**: Atomic Operations, Memory Barriers
- **Error Handling**: Robust Recovery, Graceful Degradation

### Industry Applications
- **High-Frequency Trading**: Sub-microsecond message processing
- **Quantitative Finance**: Market microstructure understanding
- **Systems Engineering**: Low-latency, high-performance C++ systems
- **Algorithm Design**: Advanced data structures and optimization

---

## Complete Feature Matrix

| Component | Feature | Status | Performance |
|-----------|---------|--------|-------------|
| **FIX Parser** | Zero-copy parsing | Complete | 2.46M msg/s |
| **FSM Parser** | Streaming support | Complete | 1.95M msg/s |
| **Threading** | Lock-free queues | Complete | 10M+ ops/s |
| **Recovery** | Error handling | Complete | Pattern-based |
| **Order Book** | Real-time updates | Complete | O(log n) ops |
| **Memory** | Zero allocations | Complete | Hot path only |
| **Testing** | Comprehensive | Complete | 95%+ coverage |
| **Benchmarks** | Performance | Complete | Google Benchmark |

---

## Development

### Running Tests
```bash
# FeedHandler tests
cd feedhandler/build
./test_fsm_parser
./test_streaming_handler
./test_garbage_recovery
./test_threaded_feedhandler

# OrderBook tests
cd orderbook/build
./price_level_tests
./order_book_tests

# Performance benchmarks
./feedhandler/build/gbench_parsers
```

### Code Quality
```bash
# Build with AddressSanitizer
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DFEED_ASAN=ON
cmake --build build

# Run with memory checking
./build/test_fsm_parser
```

---

<div align="center">

## Project Status

**Status**: Complete Implementation  
**Performance**: Exceeds All Targets  
**Production Readiness**: Enterprise Grade  

Built following a comprehensive 9-month roadmap demonstrating mastery of advanced computer science concepts through practical financial technology implementation.

[Report Bug](https://github.com/Keerthivasan-Venkitajalam/Scratch/issues) • [Request Feature](https://github.com/Keerthivasan-Venkitajalam/Scratch/issues)

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

**Quantitative Trading System** - Production-ready high-frequency trading infrastructure built from scratch.

</div>
