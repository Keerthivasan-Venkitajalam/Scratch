# FIX Protocol Reference

## Overview

The Financial Information eXchange (FIX) protocol is a messaging standard for pre-trade communications and trade execution in financial markets. This document covers the essential concepts needed for implementing a FIX parser.

## Message Structure

### Basic Format
```
tag=value<SOH>tag=value<SOH>tag=value<SOH>...
```

Where:
- `tag` is a numeric field identifier
- `value` is the field data (plain text)
- `<SOH>` is the Start of Header character (ASCII 0x01)

### Example Message
```
8=FIX.4.4|9=79|35=D|11=2636626972408080000|21=2|38=300|40=1|54=1|55=MSFT|60=20180523-18:34:07|10=020|
```
*Note: `|` represents SOH (0x01) for readability*

## Message Components

### 1. Header (Required)
- **Tag 8**: BeginString - FIX version (e.g., "FIX.4.4")
- **Tag 9**: BodyLength - Message body length in bytes
- **Tag 35**: MsgType - Message type identifier

### 2. Body (Variable)
Contains the actual message data with application-specific tags.

### 3. Trailer (Required)
- **Tag 10**: CheckSum - Three-digit checksum for message integrity

## Common Tags for Market Data

| Tag | Name | Type | Description | Example |
|-----|------|------|-------------|---------|
| 8 | BeginString | String | FIX version | `8=FIX.4.4` |
| 9 | BodyLength | Length | Message body length | `9=79` |
| 10 | CheckSum | String | Message checksum | `10=020` |
| 35 | MsgType | String | Message type | `35=D` (New Order) |
| 38 | OrderQty | Qty | Order quantity | `38=1000` |
| 44 | Price | Price | Price per unit | `44=123.45` |
| 52 | SendingTime | UTCTimestamp | Message timestamp | `52=20240131-12:34:56` |
| 54 | Side | Char | Buy/Sell indicator | `54=1` (Buy), `54=2` (Sell) |
| 55 | Symbol | String | Instrument symbol | `55=MSFT` |

## Message Types (Tag 35)

| Value | Name | Description |
|-------|------|-------------|
| D | NewOrderSingle | New order message |
| 8 | ExecutionReport | Order execution report |
| V | MarketDataRequest | Request for market data |
| W | MarketDataSnapshotFullRefresh | Market data snapshot |
| X | MarketDataIncrementalRefresh | Market data update |

## Side Values (Tag 54)

| Value | Description |
|-------|-------------|
| 1 | Buy |
| 2 | Sell |
| 3 | Buy minus |
| 4 | Sell plus |
| 5 | Sell short |
| 6 | Sell short exempt |

## Parsing Considerations

### 1. Stream Processing
- FIX messages arrive as a continuous TCP stream
- Messages may be fragmented across multiple recv() calls
- Parser must handle partial messages gracefully

### 2. Field Validation
- Required fields must be present
- Field order may vary (except header/trailer)
- Invalid checksums should be rejected

### 3. Performance Optimizations
- Avoid string allocations during parsing
- Use string_view for zero-copy field access
- Implement fast integer/decimal parsing
- Consider finite state machine for parsing

## Example Parsing Flow

```cpp
// Pseudo-code for FIX message parsing
enum ParseState {
    WAIT_TAG,
    READ_TAG, 
    WAIT_VALUE,
    READ_VALUE,
    COMPLETE
};

ParseState state = WAIT_TAG;
int current_tag = 0;
std::string_view current_value;

for (char c : message_buffer) {
    switch (state) {
        case WAIT_TAG:
            if (isdigit(c)) {
                current_tag = c - '0';
                state = READ_TAG;
            }
            break;
            
        case READ_TAG:
            if (isdigit(c)) {
                current_tag = current_tag * 10 + (c - '0');
            } else if (c == '=') {
                state = READ_VALUE;
                value_start = next_pos;
            }
            break;
            
        case READ_VALUE:
            if (c == SOH) {
                current_value = string_view(value_start, current_pos - value_start);
                process_field(current_tag, current_value);
                state = WAIT_TAG;
            }
            break;
    }
}
```

## Checksum Calculation

The checksum is calculated as the sum of all bytes in the message (excluding the checksum field itself) modulo 256.

```cpp
uint8_t calculate_checksum(const char* message, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; ++i) {
        sum += static_cast<uint8_t>(message[i]);
    }
    return sum;
}
```

## References

- [FIX Protocol Official Site](https://www.fixtrading.org/)
- [FIX 4.4 Specification](https://www.fixtrading.org/standards/fix-4-4/)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)

## Implementation Notes

For our FeedHandler implementation:
1. Focus on market data messages (types V, W, X)
2. Extract essential fields: Symbol (55), Price (44), Quantity (38), Side (54)
3. Use zero-copy parsing with string_view
4. Implement robust stream reassembly for TCP fragmentation
5. Optimize for high-frequency parsing (>1M messages/second)