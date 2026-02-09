# FIX Parser Finite State Machine (FSM) Design

## Overview

This document defines the Finite State Machine (FSM) for parsing FIX protocol messages character-by-character. The FSM enables streaming parsing capability, allowing the parser to handle fragmented TCP messages that arrive across multiple `recv()` calls.

## Why FSM?

### Current Parser Limitations
Our current parsers (naive and string_view) require complete messages:
- Must have entire message in buffer before parsing
- Cannot handle partial messages split across TCP packets
- No way to resume parsing mid-message

### FSM Advantages
1. **Streaming Capability**: Parse character-by-character, can stop and resume
2. **Fragmentation Handling**: Works with incomplete messages
3. **Memory Efficiency**: No need to buffer entire messages
4. **Predictable Performance**: Single-pass, no backtracking
5. **Error Recovery**: Can detect and recover from malformed messages

## State Machine Design

### States

```
┌─────────────┐
│   START     │ ──────────────────────────────────┐
└─────────────┘                                    │
      │                                            │
      │ (any char)                                 │
      ▼                                            │
┌─────────────┐                                    │
│  WAIT_TAG   │ ◄──────────────────────┐           │
└─────────────┘                        │           │
      │                                │           │
      │ (digit)                        │           │
      ▼                                │           │
┌─────────────┐                        │           │
│  READ_TAG   │                        │           │
└─────────────┘                        │           │
      │                                │           │
      │ ('=')                          │           │
      ▼                                │           │
┌─────────────┐                        │           │
│ WAIT_VALUE  │                        │           │
└─────────────┘                        │           │
      │                                │           │
      │ (any char)                     │           │
      ▼                                │           │
┌─────────────┐                        │           │
│ READ_VALUE  │                        │           │
└─────────────┘                        │           │
      │                                │           │
      │ (SOH/delimiter)                │           │
      ▼                                │           │
┌─────────────┐                        │           │
│   DELIM     │ ───────────────────────┘           │
└─────────────┘                                    │
      │                                            │
      │ (end of message)                           │
      ▼                                            │
┌─────────────┐                                    │
│  COMPLETE   │ ───────────────────────────────────┘
└─────────────┘
```

### State Descriptions

#### 1. START
- **Purpose**: Initial state, waiting for message to begin
- **Entry**: Parser initialization or after completing previous message
- **Exit**: First character received
- **Actions**: Reset parser state, clear buffers

#### 2. WAIT_TAG
- **Purpose**: Waiting for start of next tag
- **Entry**: From START, or after processing previous field (DELIM)
- **Exit**: First digit of tag encountered
- **Actions**: Skip whitespace, prepare for tag reading
- **Transitions**:
  - `digit` → READ_TAG
  - `SOH` → WAIT_TAG (skip empty fields)
  - `end of buffer` → WAIT_TAG (resume later)

#### 3. READ_TAG
- **Purpose**: Reading tag number (numeric identifier)
- **Entry**: First digit of tag encountered
- **Exit**: '=' character encountered
- **Actions**: Accumulate digits into tag number
- **Transitions**:
  - `digit` → READ_TAG (continue reading)
  - `'='` → WAIT_VALUE
  - `invalid char` → ERROR
  - `end of buffer` → READ_TAG (resume later)

#### 4. WAIT_VALUE
- **Purpose**: Positioned after '=', ready to read value
- **Entry**: '=' character processed
- **Exit**: First character of value encountered
- **Actions**: Mark start of value in buffer
- **Transitions**:
  - `any char` → READ_VALUE
  - `end of buffer` → WAIT_VALUE (resume later)

#### 5. READ_VALUE
- **Purpose**: Reading field value until delimiter
- **Entry**: First character of value encountered
- **Exit**: SOH (0x01) delimiter encountered
- **Actions**: Track value length, no copying
- **Transitions**:
  - `any char except SOH` → READ_VALUE
  - `SOH (0x01)` → DELIM
  - `'|' (for testing)` → DELIM
  - `end of buffer` → READ_VALUE (resume later)

#### 6. DELIM
- **Purpose**: Process completed tag-value pair
- **Entry**: Delimiter encountered after value
- **Exit**: Immediately after processing field
- **Actions**: 
  - Store tag-value pair
  - Check if message is complete (tag 10 = checksum)
- **Transitions**:
  - `more fields` → WAIT_TAG
  - `tag 10 processed` → COMPLETE
  - `buffer full` → ERROR

#### 7. COMPLETE
- **Purpose**: Message fully parsed
- **Entry**: Checksum field (tag 10) processed
- **Exit**: After returning parsed message
- **Actions**: 
  - Validate message
  - Create Tick object
  - Return to START for next message

#### 8. ERROR (implicit)
- **Purpose**: Handle malformed messages
- **Entry**: Invalid character or sequence detected
- **Actions**: 
  - Log error
  - Scan for next message start ("8=FIX")
  - Resume parsing

## State Transition Table

| Current State | Input | Next State | Action |
|--------------|-------|------------|--------|
| START | any | WAIT_TAG | Reset state |
| WAIT_TAG | digit | READ_TAG | Start tag accumulation |
| WAIT_TAG | SOH | WAIT_TAG | Skip empty field |
| WAIT_TAG | EOF | WAIT_TAG | Suspend, resume later |
| READ_TAG | digit | READ_TAG | Accumulate digit |
| READ_TAG | '=' | WAIT_VALUE | Tag complete |
| READ_TAG | other | ERROR | Invalid tag |
| READ_TAG | EOF | READ_TAG | Suspend, resume later |
| WAIT_VALUE | any | READ_VALUE | Mark value start |
| WAIT_VALUE | EOF | WAIT_VALUE | Suspend, resume later |
| READ_VALUE | non-SOH | READ_VALUE | Continue value |
| READ_VALUE | SOH/'|' | DELIM | Value complete |
| READ_VALUE | EOF | READ_VALUE | Suspend, resume later |
| DELIM | tag != 10 | WAIT_TAG | Store field, continue |
| DELIM | tag == 10 | COMPLETE | Message complete |
| COMPLETE | - | START | Ready for next message |

## Parser State Structure

```cpp
struct ParserState {
    enum State {
        START,
        WAIT_TAG,
        READ_TAG,
        WAIT_VALUE,
        READ_VALUE,
        DELIM,
        COMPLETE,
        ERROR
    };
    
    State current_state;
    int current_tag;
    const char* value_start;
    size_t value_length;
    size_t field_count;
    Field fields[MAX_FIELDS];
    
    // For resumption after buffer exhaustion
    size_t buffer_position;
    bool message_complete;
};
```

## Example: Parsing Flow

### Input Message
```
8=FIX.4.4|55=MSFT|44=123.45|10=020|
```

### Character-by-Character Processing

| Char | State Before | Action | State After | Notes |
|------|-------------|--------|-------------|-------|
| '8' | START | Start tag | READ_TAG | tag = 8 |
| '=' | READ_TAG | Tag complete | WAIT_VALUE | tag = 8 |
| 'F' | WAIT_VALUE | Start value | READ_VALUE | value_start = 'F' |
| 'I' | READ_VALUE | Continue | READ_VALUE | |
| 'X' | READ_VALUE | Continue | READ_VALUE | |
| '.' | READ_VALUE | Continue | READ_VALUE | |
| '4' | READ_VALUE | Continue | READ_VALUE | |
| '.' | READ_VALUE | Continue | READ_VALUE | |
| '4' | READ_VALUE | Continue | READ_VALUE | |
| '|' | READ_VALUE | Delimiter | DELIM | Store (8, "FIX.4.4") |
| '5' | WAIT_TAG | Start tag | READ_TAG | tag = 5 |
| '5' | READ_TAG | Continue | READ_TAG | tag = 55 |
| '=' | READ_TAG | Tag complete | WAIT_VALUE | tag = 55 |
| 'M' | WAIT_VALUE | Start value | READ_VALUE | value_start = 'M' |
| 'S' | READ_VALUE | Continue | READ_VALUE | |
| 'F' | READ_VALUE | Continue | READ_VALUE | |
| 'T' | READ_VALUE | Continue | READ_VALUE | |
| '|' | READ_VALUE | Delimiter | DELIM | Store (55, "MSFT") |
| ... | ... | ... | ... | Continue for remaining fields |
| '0' | READ_VALUE | Continue | READ_VALUE | tag 10 value |
| '|' | READ_VALUE | Delimiter | DELIM | Store (10, "020") |
| - | DELIM | Tag 10 seen | COMPLETE | Message complete |

## Handling Fragmented Messages

### Scenario: Message Split Across Two recv() Calls

#### First recv() - Partial Message
```
8=FIX.4.4|55=MS
```

**Processing:**
1. Parse: `8=FIX.4.4|` → Store (8, "FIX.4.4")
2. Parse: `55=` → Enter WAIT_VALUE state
3. Parse: `MS` → Enter READ_VALUE state
4. **Buffer exhausted** → Save state (READ_VALUE, tag=55, value_start, length=2)
5. Return INCOMPLETE status

#### Second recv() - Continuation
```
FT|44=123.45|10=020|
```

**Processing:**
1. **Resume** from saved state (READ_VALUE, tag=55)
2. Parse: `FT` → Continue READ_VALUE (now "MSFT")
3. Parse: `|` → DELIM, store (55, "MSFT")
4. Continue parsing remaining fields...
5. Complete message

### Key Implementation Details

```cpp
// Parser can suspend and resume at any point
ParseResult parse_chunk(const char* buffer, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        char c = buffer[i];
        
        switch (state) {
            case READ_TAG:
                if (isdigit(c)) {
                    current_tag = current_tag * 10 + (c - '0');
                } else if (c == '=') {
                    state = WAIT_VALUE;
                } else {
                    state = ERROR;
                }
                break;
                
            case READ_VALUE:
                if (c == SOH || c == '|') {
                    // Store field
                    store_field(current_tag, value_start, value_length);
                    state = DELIM;
                } else {
                    value_length++;
                }
                break;
                
            // ... other states
        }
    }
    
    // Return status: COMPLETE, INCOMPLETE, or ERROR
    return get_parse_result();
}
```

## Performance Characteristics

### Advantages
- **O(n) complexity**: Single pass through input
- **No backtracking**: Forward-only parsing
- **Minimal memory**: Fixed-size state structure
- **Cache-friendly**: Sequential memory access
- **Predictable**: No dynamic allocation

### Expected Performance
- **Target**: 500K+ messages/second
- **Latency**: <2 μs per message
- **Memory**: ~1KB state structure
- **Scalability**: Constant memory regardless of message size

## Error Handling

### Error Detection
- Invalid characters in tag (non-digit before '=')
- Missing '=' after tag
- Buffer overflow (too many fields)
- Invalid message structure

### Error Recovery Strategy
1. **Detect error**: Invalid state transition
2. **Log error**: Record position and context
3. **Scan forward**: Look for "8=FIX" (message start)
4. **Resume parsing**: Start fresh from next message
5. **Report**: Return error status with partial data

## Implementation Checklist

- [ ] Define State enum
- [ ] Create ParserState struct
- [ ] Implement state transition logic
- [ ] Add buffer position tracking
- [ ] Handle EOF/incomplete buffer
- [ ] Implement field storage
- [ ] Add message completion detection
- [ ] Create Tick from parsed fields
- [ ] Add error handling
- [ ] Test with fragmented messages
- [ ] Benchmark performance

## Testing Strategy

### Unit Tests
1. **Complete messages**: Parse full messages in one call
2. **Fragmented messages**: Split at every possible position
3. **Multiple messages**: Parse stream of messages
4. **Error cases**: Invalid tags, missing delimiters
5. **Edge cases**: Empty values, very long values

### Fragmentation Test Cases
```cpp
// Test 1: Split in middle of tag
"8=FIX.4.4|5" + "5=MSFT|10=020|"

// Test 2: Split in middle of value
"8=FIX.4.4|55=MS" + "FT|10=020|"

// Test 3: Split at delimiter
"8=FIX.4.4|55=MSFT" + "|10=020|"

// Test 4: Split between fields
"8=FIX.4.4|" + "55=MSFT|10=020|"

// Test 5: Multiple splits
"8=FI" + "X.4.4|55=" + "MSFT|10=" + "020|"
```

## Next Steps

1. **Day 16**: Implement FSM parser in C++
2. **Day 17**: Add tag switch optimization
3. **Day 18**: Integrate with receive buffer
4. **Day 19**: Write comprehensive unit tests
5. **Day 20**: Benchmark and optimize

## References

- [Lexical Analysis Basics](lexical_analysis_basics.md)
- [FIX Protocol Reference](fix_protocol_reference.md)
- [Zero-Copy Parsing Guide](zero_copy_parsing_guide.md)

---

**Document Status**: Complete  
**Week**: 3, Day 1 (Day 15)  
**Next**: FSM Implementation (Day 16)