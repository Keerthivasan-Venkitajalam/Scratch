# FeedHandler

A high-performance C++ library for parsing FIX protocol market data messages with zero-copy design and minimal memory allocations.

## Overview

FeedHandler is a low-latency parser designed for processing financial market data streams. It implements multiple parsing strategies optimized for different use cases, from simple message parsing to streaming TCP data with fragmentation handling.

## Key Features

- **Zero-copy parsing** - Direct buffer references without memory allocation
- **Multiple parser implementations** - Choose the right parser for your use case
- **Streaming support** - Handle fragmented TCP streams seamlessly
- **High throughput** - Optimized for processing millions of messages per second
- **Type-safe** - Modern C++20 with strong type safety
- **Extensible** - Easy to add support for additional FIX tags

## Building

```bash
cd feedhandler
mkdir -p build && cd build
cmake ..
make
```

### Build Options

```bash
# Release build (optimized)
cmake -DCMAKE_BUILD_TYPE=Release ..

# Enable AddressSanitizer for debugging
cmake -DFEED_ASAN=ON ..
```

## Usage

### Basic Message Parsing

```cpp
#include "parser/fsm_fix_parser.hpp"

feedhandler::parser::FSMFixParser parser;
std::vector<feedhandler::common::Tick> ticks;

const char* message = "8=FIX.4.4|55=MSFT|44=123.45|38=1000|54=1|10=020|\n";
parser.parse(message, strlen(message), ticks);

for (const auto& tick : ticks) {
    std::cout << tick.symbol << " $" 
              << feedhandler::common::price_to_double(tick.price)
              << " qty:" << tick.qty << std::endl;
}
```

### Streaming TCP Data

```cpp
#include "parser/streaming_fix_handler.hpp"

feedhandler::parser::StreamingFixHandler handler;
std::vector<feedhandler::common::Tick> ticks;

// Process data as it arrives from network
handler.process_data(buffer, bytes_received, ticks);
```

## Architecture

```
feedhandler/
├── include/
│   ├── common/          # Core data structures
│   ├── net/             # Network utilities
│   └── parser/          # Parser implementations
├── src/
│   ├── net/
│   ├── parser/
│   └── main.cpp
├── docs/                # Technical documentation
└── tests/               # Test programs
```

## Supported FIX Tags

- Tag 8: BeginString
- Tag 35: MsgType
- Tag 38: OrderQty
- Tag 44: Price
- Tag 54: Side
- Tag 55: Symbol
- Tag 10: CheckSum

## Performance

The library includes multiple parser implementations with different performance characteristics:

- **Naive Parser**: Simple implementation using standard library
- **String_view Parser**: Zero-allocation parsing with string views
- **Optimized Parser**: Custom number parsing and optimizations
- **FSM Parser**: Streaming-capable finite state machine

Run benchmarks to compare performance on your hardware:

```bash
./parser_benchmark
```

## Testing

```bash
# Run test programs
./test_fsm_parser
./test_streaming_handler
./test_fast_number_parser
```

## Documentation

See `feedhandler/docs/` for detailed documentation:

- [FIX Protocol Reference](feedhandler/docs/fix_protocol_reference.md)
- [Tick Specification](feedhandler/docs/tick_spec.md)
- [Parser Implementations](feedhandler/docs/)

## Requirements

- C++20 compatible compiler (clang++ or g++)
- CMake 3.15 or higher
- POSIX-compliant system (Linux, macOS)
