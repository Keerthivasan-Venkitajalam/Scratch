# Zero-Copy Parsing: Theory and Practice

## Overview

Zero-copy parsing is a technique that eliminates unnecessary memory allocations and data copying during the parsing process. This document explores the theory behind zero-copy parsing and demonstrates practical implementation strategies based on our FIX parser development.

## Table of Contents

1. [What is Zero-Copy Parsing?](#what-is-zero-copy-parsing)
2. [Why Zero-Copy Matters](#why-zero-copy-matters)
3. [Core Principles](#core-principles)
4. [Implementation Strategies](#implementation-strategies)
5. [Trade-offs and Considerations](#trade-offs-and-considerations)
6. [Practical Examples](#practical-examples)
7. [Performance Analysis](#performance-analysis)

---

## What is Zero-Copy Parsing?

Zero-copy parsing is a parsing technique where the parser **references data directly in the input buffer** rather than copying it into new memory locations. Instead of creating new strings or data structures, the parser returns views or pointers into the original buffer.

### Traditional Parsing (With Copies)
```cpp
// ❌ Traditional approach - multiple copies
std::string input = "tag=value|tag2=value2|";
std::vector<std::string> fields;

// Copy 1: Extract substring
std::string field = input.substr(0, 9);  // "tag=value"

// Copy 2: Split into parts
std::string tag = field.substr(0, 3);    // "tag"
std::string value = field.substr(4, 5);  // "value"

// Copy 3: Store in container
fields.push_back(value);  // Another copy
```

**Memory allocations**: 4+ heap allocations per field

### Zero-Copy Parsing (No Copies)
```cpp
// ✅ Zero-copy approach - no allocations
std::string_view input = "tag=value|tag2=value2|";

// No copies - just pointer arithmetic
std::string_view field = input.substr(0, 9);  // Points into input
std::string_view tag = field.substr(0, 3);    // Points into input
std::string_view value = field.substr(4, 5);  // Points into input

// Store view (just pointer + length)
std::vector<std::string_view> fields;
fields.push_back(value);  // No copy, just 16 bytes
```

**Memory allocations**: 0 heap allocations

---

## Why Zero-Copy Matters

### Performance Impact

From our FIX parser benchmarks:

| Parser Type | Throughput | Heap Allocations | Speedup |
|-------------|-----------|------------------|---------|
| Naive (with copies) | 112K msgs/sec | ~200 bytes/msg | 1.0x |
| Zero-copy | 473K msgs/sec | 0 bytes/msg | **4.2x** |

### Benefits

1. **Speed**: Eliminates allocation/deallocation overhead
2. **Memory**: Reduces memory footprint dramatically
3. **Cache**: Better cache locality (data stays in one place)
4. **Latency**: More predictable performance (no GC pauses)
5. **Throughput**: Can process more messages per second

### Cost Savings

For a high-frequency trading system processing 1 billion messages/day:

- **Naive parser**: ~200 GB of temporary allocations
- **Zero-copy parser**: ~0 GB of temporary allocations

This translates to:
- Reduced memory bandwidth usage
- Lower CPU cache pressure
- Fewer memory allocator calls
- Better power efficiency

---

## Core Principles

### 1. Reference, Don't Copy

**Principle**: Use pointers or views to reference data in the original buffer.

```cpp
// ❌ Bad: Copy data
std::string extract_symbol(const std::string& message) {
    size_t start = message.find("55=") + 3;
    size_t end = message.find('|', start);
    return message.substr(start, end - start);  // COPY!
}

// ✅ Good: Return view
std::string_view extract_symbol(std::string_view message) {
    size_t start = message.find("55=") + 3;
    size_t end = message.find('|', start);
    return message.substr(start, end - start);  // NO COPY!
}
```

### 2. Lifetime Management

**Principle**: The original buffer must outlive all views into it.

```cpp
// ❌ DANGEROUS: Buffer destroyed before view is used
std::string_view get_symbol() {
    std::string buffer = "55=MSFT|";
    return buffer.substr(3, 4);  // Returns view into buffer
}  // buffer destroyed here!
// Returned string_view is now DANGLING

// ✅ SAFE: Buffer outlives view
std::string_view get_symbol(const std::string& buffer) {
    return buffer.substr(3, 4);  // View into caller's buffer
}  // buffer still valid in caller
```

### 3. Stack Over Heap

**Principle**: Use stack-allocated data structures when possible.

```cpp
// ❌ Heap allocation
std::map<int, std::string> fields;  // Heap-allocated nodes
fields[55] = "MSFT";                // Heap-allocated string

// ✅ Stack allocation
struct Field { int tag; std::string_view value; };
Field fields[32];  // Stack-allocated array
fields[0] = {55, "MSFT"};  // No heap allocation
```

### 4. In-Place Parsing

**Principle**: Parse data where it sits, don't move it.

```cpp
// ❌ Move data to parse
std::stringstream ss(message);
std::string field;
while (std::getline(ss, field, '|')) {  // Copies each field
    process(field);
}

// ✅ Parse in place
size_t pos = 0;
while (pos < message.size()) {
    size_t end = message.find('|', pos);
    std::string_view field = message.substr(pos, end - pos);
    process(field);  // No copy
    pos = end + 1;
}
```

---

## Implementation Strategies

### Strategy 1: String Views

Use `std::string_view` (C++17) to reference substrings without copying.

```cpp
class ZeroCopyParser {
    struct Field {
        int tag;
        std::string_view value;  // Points into original buffer
    };
    
    static Field parse_field(std::string_view input) {
        size_t eq = input.find('=');
        int tag = parse_int(input.substr(0, eq));
        std::string_view value = input.substr(eq + 1);
        return {tag, value};
    }
};
```

**Pros**: Simple, standard library, type-safe  
**Cons**: Requires C++17, lifetime management needed

### Strategy 2: Pointer + Length

Use raw pointers with explicit length tracking.

```cpp
struct StringRef {
    const char* data;
    size_t length;
    
    StringRef substr(size_t pos, size_t len) const {
        return {data + pos, len};
    }
};
```

**Pros**: Works in C++11, explicit control  
**Cons**: More error-prone, no bounds checking

### Strategy 3: Iterator Pairs

Use begin/end iterator pairs to define ranges.

```cpp
template<typename Iterator>
struct Range {
    Iterator begin;
    Iterator end;
    
    size_t size() const { return std::distance(begin, end); }
};
```

**Pros**: Generic, works with any container  
**Cons**: More complex, requires template knowledge

### Strategy 4: Arena Allocation

Pre-allocate a large buffer and sub-allocate from it.

```cpp
class Arena {
    char buffer[1024 * 1024];  // 1MB arena
    size_t offset = 0;
    
public:
    char* allocate(size_t size) {
        char* ptr = buffer + offset;
        offset += size;
        return ptr;
    }
    
    void reset() { offset = 0; }  // Bulk free
};
```

**Pros**: Fast allocation, good cache locality  
**Cons**: Fixed size, manual memory management

---

## Trade-offs and Considerations

### Advantages of Zero-Copy

✅ **Performance**: 2-5x faster parsing  
✅ **Memory**: Zero heap allocations  
✅ **Predictability**: No allocator variability  
✅ **Scalability**: Better with high message rates  
✅ **Cache**: Improved cache utilization  

### Disadvantages of Zero-Copy

❌ **Complexity**: Lifetime management required  
❌ **Safety**: Dangling references possible  
❌ **Flexibility**: Buffer must remain valid  
❌ **Debugging**: Harder to track data flow  
❌ **Ownership**: Unclear data ownership  

### When to Use Zero-Copy

**Good Use Cases**:
- High-frequency message parsing
- Network protocol parsing
- Log file processing
- Real-time data streams
- Memory-constrained systems

**Poor Use Cases**:
- Long-lived data structures
- Data that needs modification
- Multi-threaded sharing
- Complex ownership patterns

---

## Practical Examples

### Example 1: FIX Message Parser

Our implementation from Week 2:

```cpp
common::Tick StringViewFixParser::parse_message(std::string_view message) {
    // Stack-allocated field storage (no heap)
    constexpr size_t MAX_FIELDS = 32;
    Field fields[MAX_FIELDS];
    
    // Extract fields (zero-copy)
    size_t field_count = extract_fields(message, fields, MAX_FIELDS);
    
    common::Tick tick;
    
    // Symbol points directly into input buffer
    if (const Field* field = find_field(fields, field_count, 55)) {
        tick.symbol = field->value;  // Zero-copy reference
    }
    
    return tick;
}
```

**Key Points**:
- Stack-allocated field array
- `string_view` references into input
- No heap allocations
- 4.2x faster than naive approach

### Example 2: CSV Parser

```cpp
std::vector<std::string_view> parse_csv_line(std::string_view line) {
    std::vector<std::string_view> fields;
    size_t start = 0;
    
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == ',') {
            fields.push_back(line.substr(start, i - start));
            start = i + 1;
        }
    }
    
    // Last field
    if (start < line.size()) {
        fields.push_back(line.substr(start));
    }
    
    return fields;
}
```

### Example 3: HTTP Header Parser

```cpp
struct HttpHeader {
    std::string_view name;
    std::string_view value;
};

HttpHeader parse_header(std::string_view line) {
    size_t colon = line.find(':');
    if (colon == std::string_view::npos) {
        return {{}, {}};
    }
    
    std::string_view name = line.substr(0, colon);
    std::string_view value = line.substr(colon + 2);  // Skip ": "
    
    return {name, value};
}
```

---

## Performance Analysis

### Memory Allocation Overhead

Typical `malloc()` overhead:
- **Time**: 50-200 CPU cycles
- **Space**: 8-16 bytes per allocation (metadata)
- **Fragmentation**: Can waste 10-30% of memory

For 1 million messages:
- **Naive parser**: 1M allocations × 150 cycles = 150M cycles wasted
- **Zero-copy parser**: 0 allocations = 0 cycles wasted

### Cache Performance

**Cache line size**: 64 bytes (typical)

**Naive parser**:
- Data scattered across memory
- Poor spatial locality
- More cache misses

**Zero-copy parser**:
- Data contiguous in buffer
- Excellent spatial locality
- Fewer cache misses

### Benchmark Results

From our implementation:

```
Naive Parser:     112,262 msgs/sec  (8.91 μs/msg)
String_view:      473,181 msgs/sec  (2.11 μs/msg)
Optimized:        581,395 msgs/sec  (1.72 μs/msg)
```

**Improvement breakdown**:
- Eliminating allocations: ~60% of speedup
- Better cache usage: ~25% of speedup
- Optimized parsing: ~15% of speedup

---

## Best Practices

### 1. Document Lifetime Requirements

```cpp
/**
 * @brief Parse FIX message
 * @param message Input buffer
 * @return Parsed tick
 * @warning The input buffer must remain valid for the lifetime
 *          of the returned Tick object
 */
Tick parse_message(std::string_view message);
```

### 2. Use RAII for Buffer Management

```cpp
class MessageBuffer {
    std::unique_ptr<char[]> buffer_;
    size_t size_;
    
public:
    MessageBuffer(size_t size) 
        : buffer_(new char[size]), size_(size) {}
    
    std::string_view view() const {
        return {buffer_.get(), size_};
    }
};
```

### 3. Validate Before Parsing

```cpp
bool is_valid_message(std::string_view msg) {
    return msg.size() >= 10 && 
           msg.substr(0, 2) == "8=" &&
           msg.back() == '|';
}
```

### 4. Provide Safe Defaults

```cpp
std::string_view safe_find_field(std::string_view msg, int tag) {
    auto field = find_field(msg, tag);
    return field ? field->value : std::string_view{};
}
```

---

## Conclusion

Zero-copy parsing is a powerful technique for high-performance systems. Our FIX parser implementation demonstrates:

- **4.2x performance improvement** over naive approach
- **Zero heap allocations** during parsing
- **473K messages/second** throughput
- **Practical lifetime management** strategies

The key is understanding the trade-offs and applying zero-copy techniques where they provide the most benefit: high-frequency, short-lived data processing.

---

## Further Reading

- [C++ string_view documentation](https://en.cppreference.com/w/cpp/string/basic_string_view)
- [Zero-copy networking in Linux](https://www.kernel.org/doc/html/latest/networking/msg_zerocopy.html)
- [Memory allocator performance](https://www.youtube.com/watch?v=nZNd5FjSquk)
- [Cache-friendly code](https://www.youtube.com/watch?v=WDIkqP4JbkE)

## Related Documents

- [FIX Protocol Reference](fix_protocol_reference.md)
- [String_view Parser Benchmark](stringview_parser_benchmark.md)
- [Optimized Parser Benchmark](optimized_parser_benchmark.md)