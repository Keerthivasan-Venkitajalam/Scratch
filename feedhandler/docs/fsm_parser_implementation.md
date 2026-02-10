# FSM FIX Parser Implementation

## Overview

This document describes the implementation of the Finite State Machine (FSM) FIX parser, which provides streaming capability for handling fragmented TCP messages. This is Week 3 Day 2 (Day 16) of the FeedHandler development.

## Implementation Details

### State Machine Design

The parser implements a character-by-character state machine with the following states:

```
WAIT_TAG → READ_TAG → WAIT_VALUE → READ_VALUE → COMPLETE
    ↑                                                ↓
    └────────────────────────────────────────────────┘
```

#### State Descriptions

1. **WAIT_TAG**: Waiting for the start of a new tag (digit character)
2. **READ_TAG**: Accumulating tag digits until '=' separator
3. **WAIT_VALUE**: Transition state (currently unused, falls through to READ_VALUE)
4. **READ_VALUE**: Accumulating value characters until delimiter ('|', SOH, '\n')
5. **COMPLETE**: Message parsing complete, ready for next message

### Key Features

#### 1. Streaming Capability
- **Can stop mid-message and resume**: Parser maintains state between calls
- **Handles fragmented TCP streams**: Messages can be split across multiple recv() calls
- **No message buffering required**: Processes data as it arrives

#### 2. Zero-Copy Design (Partial)
- **Symbol storage**: Copied to persistent buffer (64 bytes)
- **Other fields**: Parsed directly from temporary buffer
- **No heap allocations**: All buffers are stack-allocated

#### 3. Character-by-Character Processing
- **Single while loop**: Processes input one character at a time
- **switch(state)**: State transitions based on current state
- **Immediate field processing**: Fields processed as soon as delimiter is encountered

## Performance Results

### Benchmark Results

| Messages | Total Time (μs) | Messages/Second | μs/Message |
|----------|----------------|-----------------|------------|
| 1,000    | 1,899          | 526,592         | 1.90       |
| 10,000   | 23,085         | 433,181         | 2.31       |
| 100,000  | 146,933        | 680,582         | 1.47       |
| 1,000,000| 1,426,469      | **701,031**     | **1.43**   |

### Performance Comparison

| Parser Type | Messages/Second | Speedup vs Naive |
|-------------|----------------|------------------|
| Naive       | 112,262        | 1.0x             |
| String_view | 473,181        | 4.2x             |
| **FSM**     | **701,031**    | **6.2x**         |

### Key Performance Characteristics
- **Peak Throughput**: 701K messages/second
- **Average Latency**: 1.43 microseconds per message
- **Speedup**: 6.2x faster than naive parser, 1.5x faster than string_view parser
- **Streaming Overhead**: Minimal (chunked parsing at 1KB chunks)

## Implementation Structure

### Class Structure

```cpp
class FSMFixParser {
public:
    enum class State {
        WAIT_TAG, READ_TAG, WAIT_VALUE, READ_VALUE, COMPLETE
    };
    
    size_t parse(const char* buffer, size_t length, 
                 std::vector<common::Tick>& ticks);
    void reset();
    bool is_parsing() const;
    
private:
    bool process_char(char c);
    void finalize_message();
    
    State state_;
    int current_tag_;
    char value_buffer_[256];
    char tag_buffer_[16];
    TickBuilder tick_builder_;
};
```

### TickBuilder Structure

```cpp
struct TickBuilder {
    char symbol_storage[64];  // Persistent symbol storage
    size_t symbol_length;
    int64_t price;
    int32_t qty;
    char side;
    bool has_symbol, has_price, has_qty, has_side;
    
    std::string_view get_symbol() const;
    bool is_valid() const;
    void reset();
};
```

## Streaming Capability Demonstration

### Test 1: Complete Message
```cpp
Input: "8=FIX.4.4|35=D|55=AAPL|44=150.25|38=500|54=1|10=123|\n"
Result: 1 tick parsed successfully
```

### Test 2: Fragmented Message
```cpp
Fragment 1: "8=FIX.4.4|35=D|55=GO"      // Partial symbol
Fragment 2: "OGL|44=2750."               // Symbol complete, partial price
Fragment 3: "80|38=100|54=2|10="         // Price complete, partial checksum
Fragment 4: "456|\n"                     // Message complete

Result: 1 tick parsed (GOOGL $2750.80 qty:100 side:S)
```

### Test 3: Multiple Messages in Buffer
```cpp
Input: 3 complete messages in single buffer
Result: 3 ticks parsed successfully
```

### Test 4: Partial Message with Resume
```cpp
Chunk 1: "8=FIX.4.4|35=D|55=BTC-USD|44=45"  // Stops mid-price
Chunk 2: "123.75|38=50|54=2|10=999|\n"      // Resumes and completes

Result: 1 tick parsed (BTC-USD $45123.75 qty:50 side:S)
```

## State Transition Logic

### Character Processing Flow

```cpp
bool process_char(char c) {
    switch (state_) {
        case WAIT_TAG:
            if (isdigit(c)) {
                tag_buffer_[0] = c;
                state_ = READ_TAG;
            }
            break;
            
        case READ_TAG:
            if (isdigit(c)) {
                tag_buffer_[tag_length_++] = c;
            } else if (c == '=') {
                current_tag_ = parse_tag();
                state_ = READ_VALUE;
            }
            break;
            
        case READ_VALUE:
            if (is_delimiter(c)) {
                process_field(current_tag_, value_buffer_);
                state_ = WAIT_TAG;
                if (message_complete()) return true;
            } else {
                value_buffer_[value_length_++] = c;
            }
            break;
    }
    return false;
}
```

## Field Processing

### Supported FIX Tags

| Tag | Field Name | Processing |
|-----|------------|------------|
| 55  | Symbol     | Copy to symbol_storage (persistent) |
| 44  | Price      | Parse to fixed-point int64_t |
| 38  | Quantity   | Parse to int32_t |
| 54  | Side       | Convert to 'B' or 'S' |
| 10  | Checksum   | Triggers message completion |

### Number Parsing

Uses `FastNumberParser` for optimal performance:
- **Integer parsing**: `fast_atoi()` - no exceptions, overflow protection
- **Decimal parsing**: `fast_atof_fixed()` - fixed-point arithmetic, no floating-point

## Memory Management

### Buffer Sizes
- **value_buffer_**: 256 bytes (current field value)
- **tag_buffer_**: 16 bytes (current tag digits)
- **symbol_storage**: 64 bytes (persistent symbol)

### Memory Characteristics
- **Stack-only allocation**: No heap allocations during parsing
- **Fixed buffer sizes**: Predictable memory usage
- **Symbol persistence**: Copied to avoid dangling references

## Advantages Over Previous Parsers

### vs Naive Parser
- **6.2x faster**: 701K vs 112K messages/second
- **Zero heap allocations**: vs ~200 bytes/message
- **Streaming capable**: Can handle fragmented messages
- **State preservation**: Can pause and resume parsing

### vs String_view Parser
- **1.5x faster**: 701K vs 473K messages/second
- **Streaming capable**: String_view requires complete messages
- **Fragmentation handling**: Can process partial messages
- **Better for TCP streams**: Designed for network data

## Limitations and Trade-offs

### Current Limitations
1. **Symbol copying**: 64-byte copy per message (not zero-copy for symbols)
2. **Fixed buffer sizes**: May truncate very long fields
3. **Linear field search**: O(n) for field lookup (acceptable for small field counts)
4. **No checksum validation**: Checksum field triggers completion but isn't validated

### Design Trade-offs
- **Symbol persistence vs zero-copy**: Chose persistence for safety
- **Character-by-character vs batch**: Chose char-by-char for streaming
- **State complexity vs performance**: Balanced for maintainability

## Future Optimizations

### Week 3 Day 3 (Day 17): Tag Switch Optimization
- Convert tag characters to int tag_id
- Use switch(tag_id) for O(1) field assignment
- Target: Additional 10-15% performance improvement

### Week 3 Day 4 (Day 18): Buffer Integration
- Integrate with receive buffer from Week 1
- Handle TCP stream reassembly
- Circular buffer management

### Week 4: Advanced Optimizations
- SIMD for delimiter scanning
- Branch prediction hints
- Cache-line alignment
- Target: >1M messages/second

## Implementation Files

- **Header**: `include/parser/fsm_fix_parser.hpp`
- **Implementation**: `src/parser/fsm_fix_parser.cpp`
- **Test Suite**: `src/test_fsm_parser.cpp`

## Conclusion

The FSM parser successfully achieves the Week 3 Day 2 goals:

✅ **Single while loop** with character-by-character processing  
✅ **switch(state)** for state machine transitions  
✅ **Can stop mid-message and resume** - full streaming capability  
✅ **701K messages/second** - 6.2x faster than naive parser  
✅ **Zero heap allocations** - all stack-based buffers  
✅ **Handles fragmented TCP streams** - production-ready  

This implementation provides a solid foundation for high-frequency trading applications requiring robust, high-performance FIX message parsing.