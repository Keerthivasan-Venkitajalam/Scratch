# Tick Data Structure Specification

## Overview

This document defines the `Tick` struct used in the FeedHandler to represent market data events. The design prioritizes zero-copy parsing and minimal memory footprint for high-frequency trading applications.

## FIX Protocol Background

The Financial Information eXchange (FIX) protocol uses a tag=value format where:
- Messages are structured as `tag=value` pairs
- Fields are separated by SOH (Start of Header, ASCII 0x01) 
- Each tag is a numeric identifier for a specific field
- Values are plain text representations

### Common FIX Tags for Market Data

| Tag | Field Name | Description | Example |
|-----|------------|-------------|---------|
| 55  | Symbol     | Instrument identifier | `55=MSFT` |
| 44  | Price      | Price value | `44=123.45` |
| 38  | OrderQty   | Quantity/Size | `38=1000` |
| 54  | Side       | Buy/Sell indicator | `54=1` (Buy), `54=2` (Sell) |
| 52  | SendingTime| Timestamp | `52=20240131-12:34:56.789` |

### Example FIX Message
```
8=FIX.4.4|35=D|55=MSFT|44=123.45|38=1000|54=1|52=20240131-12:34:56|10=123|
```
*Note: `|` represents SOH (0x01) delimiter*

## Tick Struct Design

```cpp
struct Tick {
    std::string_view symbol;  // Instrument symbol (e.g., "MSFT", "BTC-USD")
    int64_t price;           // Price in fixed-point (scaled by 10000)
    int32_t qty;             // Quantity/size
    char side;               // 'B' for Buy, 'S' for Sell
    uint64_t timestamp;      // Nanoseconds since Unix epoch
};
```

## Design Rationale

### 1. Zero-Copy Symbol Storage
- `std::string_view symbol` points directly into the receive buffer
- No heap allocation for symbol strings
- Requires buffer lifetime management

### 2. Fixed-Point Price Representation
- `int64_t price` stores price * 10000 (4 decimal places)
- Avoids floating-point precision issues
- Example: $123.4567 → 1234567
- Range: ±922,337,203,685,477.5807

### 3. Compact Side Representation
- `char side` uses single byte
- 'B' = Buy/Bid, 'S' = Sell/Ask
- Maps from FIX tag 54: 1→'B', 2→'S'

### 4. High-Resolution Timestamps
- `uint64_t timestamp` in nanoseconds
- Supports microsecond trading precision
- Range: ~584 years from Unix epoch

## Memory Layout

```
Total size: 32 bytes (64-bit system)
┌─────────────────┬──────────┬─────────┬──────┬─────────────┐
│ symbol (16B)    │ price(8B)│ qty(4B) │side  │timestamp(8B)│
│ ptr + size      │          │         │(1B)  │             │
└─────────────────┴──────────┴─────────┴──────┴─────────────┘
```

## Usage Examples

### Creating a Tick
```cpp
// From parsed FIX message buffer
const char* buffer = "55=MSFT\x01" "44=123.4500\x01" "38=1000\x01" "54=1\x01";
Tick tick;
tick.symbol = std::string_view(symbol_start, symbol_len);
tick.price = 1234500;  // 123.4500 * 10000
tick.qty = 1000;
tick.side = 'B';
tick.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
```

### Price Conversion Utilities
```cpp
// Convert fixed-point to double (for display only)
double price_to_double(int64_t fixed_price) {
    return static_cast<double>(fixed_price) / 10000.0;
}

// Convert string price to fixed-point
int64_t parse_price(std::string_view price_str) {
    // Implementation in fast_atof function
    // "123.4500" → 1234500
}
```

## Buffer Lifetime Management

**Critical**: The `symbol` field contains a `string_view` that points into the receive buffer. The buffer must remain valid for the lifetime of the Tick object.

### Safe Usage Pattern
```cpp
// ✅ SAFE: Buffer outlives tick
char buffer[8192];
recv(socket, buffer, sizeof(buffer));
Tick tick = parse_message(buffer);
process_tick(tick);  // Use tick before buffer is overwritten

// ❌ UNSAFE: Buffer may be overwritten
std::vector<Tick> ticks;
char buffer[8192];
while (true) {
    recv(socket, buffer, sizeof(buffer));
    ticks.push_back(parse_message(buffer));  // Dangling string_view!
    // Next recv() overwrites buffer, invalidating all previous ticks
}
```

## Performance Characteristics

- **Size**: 32 bytes per tick (cache-friendly)
- **Allocation**: Zero heap allocations during parsing
- **Precision**: 4 decimal places for prices
- **Range**: Supports all realistic financial instrument prices
- **Throughput**: Optimized for >1M ticks/second parsing

## Future Extensions

Potential fields for enhanced market data:
- `uint32_t exchange_id` - Exchange identifier
- `uint16_t msg_type` - Message type (trade, quote, etc.)
- `int64_t bid_price, ask_price` - Full order book top
- `uint32_t sequence_num` - Message sequence number

## Implementation Files

- Header: `include/common/tick.hpp`
- Implementation: `src/common/tick.cpp` (utilities)
- Parser integration: `src/parser/fix_parser.cpp`