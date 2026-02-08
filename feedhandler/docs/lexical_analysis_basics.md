# Lexical Analysis Basics

## Overview

Lexical analysis (also called lexing or tokenization) is the first phase of parsing. It converts a stream of characters into a stream of tokens. This document covers the fundamentals of lexical analysis and how they apply to our FIX protocol parser.

## Table of Contents

1. [What is Lexical Analysis?](#what-is-lexical-analysis)
2. [Tokens and Lexemes](#tokens-and-lexemes)
3. [Finite State Machines](#finite-state-machines)
4. [Pattern Matching](#pattern-matching)
5. [Practical Implementation](#practical-implementation)
6. [FIX Protocol Lexical Analysis](#fix-protocol-lexical-analysis)

---

## What is Lexical Analysis?

Lexical analysis is the process of converting a sequence of characters into a sequence of tokens. It's the first step in parsing and serves as the interface between the raw input and the parser.

### The Parsing Pipeline

```
Raw Input → Lexical Analysis → Syntax Analysis → Semantic Analysis
   ↓              ↓                   ↓                  ↓
"55=MSFT|"    [TAG:55][EQ][VAL:MSFT][DELIM]    Tick{symbol:"MSFT"}
```

### Example: FIX Message

**Input**: `55=MSFT|44=123.45|`

**Tokens**:
1. `TAG(55)`
2. `EQUALS`
3. `VALUE("MSFT")`
4. `DELIMITER`
5. `TAG(44)`
6. `EQUALS`
7. `VALUE("123.45")`
8. `DELIMITER`

---

## Tokens and Lexemes

### Definitions

**Token**: A categorized unit of meaning
- Type: What kind of thing it is (TAG, VALUE, DELIMITER)
- Value: The actual data (55, "MSFT", |)

**Lexeme**: The actual character sequence that forms a token
- `55` is the lexeme for token `TAG(55)`
- `MSFT` is the lexeme for token `VALUE("MSFT")`

### Token Structure

```cpp
enum class TokenType {
    TAG,        // Numeric field identifier
    EQUALS,     // '=' separator
    VALUE,      // Field value
    DELIMITER,  // '|' or SOH (0x01)
    END         // End of input
};

struct Token {
    TokenType type;
    std::string_view lexeme;  // Points to original input
    int line;                 // For error reporting
    int column;               // For error reporting
};
```

### Example Tokenization

```cpp
// Input
std::string_view input = "55=MSFT|44=123.45|";

// Tokens
std::vector<Token> tokens = {
    {TokenType::TAG,       "55",     1, 0},
    {TokenType::EQUALS,    "=",      1, 2},
    {TokenType::VALUE,     "MSFT",   1, 3},
    {TokenType::DELIMITER, "|",      1, 7},
    {TokenType::TAG,       "44",     1, 8},
    {TokenType::EQUALS,    "=",      1, 10},
    {TokenType::VALUE,     "123.45", 1, 11},
    {TokenType::DELIMITER, "|",      1, 17},
    {TokenType::END,       "",       1, 18}
};
```

---

## Finite State Machines

A Finite State Machine (FSM) is a mathematical model used to design lexical analyzers. It consists of:
- **States**: Different modes the lexer can be in
- **Transitions**: Rules for moving between states
- **Input**: Characters from the input stream
- **Actions**: What to do in each state

### FSM for FIX Protocol

```
States:
  START      → Initial state
  READ_TAG   → Reading numeric tag
  WAIT_EQ    → Expecting '='
  READ_VALUE → Reading field value
  WAIT_DELIM → Expecting '|'

Transitions:
  START + digit     → READ_TAG
  READ_TAG + digit  → READ_TAG
  READ_TAG + '='    → WAIT_EQ
  WAIT_EQ + any     → READ_VALUE
  READ_VALUE + '|'  → WAIT_DELIM
  WAIT_DELIM + digit → READ_TAG
```

### State Diagram

```
    ┌─────┐
    │START│
    └──┬──┘
       │ digit
       ▼
  ┌─────────┐
  │READ_TAG │◄──┐
  └────┬────┘   │ digit
       │ '='    │
       ▼        │
  ┌─────────┐  │
  │ WAIT_EQ │  │
  └────┬────┘  │
       │ any   │
       ▼       │
┌────────────┐ │
│READ_VALUE  │ │
└─────┬──────┘ │
      │ '|'    │
      ▼        │
┌────────────┐ │
│WAIT_DELIM  │─┘
└────────────┘
```

### FSM Implementation

```cpp
enum class State {
    START,
    READ_TAG,
    WAIT_EQ,
    READ_VALUE,
    WAIT_DELIM
};

class Lexer {
    State state = State::START;
    std::string_view input;
    size_t pos = 0;
    
public:
    Token next_token() {
        while (pos < input.size()) {
            char c = input[pos];
            
            switch (state) {
                case State::START:
                    if (isdigit(c)) {
                        state = State::READ_TAG;
                        // Start collecting tag
                    }
                    break;
                    
                case State::READ_TAG:
                    if (isdigit(c)) {
                        // Continue collecting tag
                    } else if (c == '=') {
                        state = State::WAIT_EQ;
                        return make_tag_token();
                    }
                    break;
                    
                case State::WAIT_EQ:
                    state = State::READ_VALUE;
                    // Start collecting value
                    break;
                    
                case State::READ_VALUE:
                    if (c == '|') {
                        state = State::WAIT_DELIM;
                        return make_value_token();
                    }
                    // Continue collecting value
                    break;
                    
                case State::WAIT_DELIM:
                    state = State::START;
                    return make_delimiter_token();
            }
            
            ++pos;
        }
        
        return {TokenType::END, "", 0, 0};
    }
};
```

---

## Pattern Matching

Lexical analysis often uses pattern matching to identify tokens. Patterns can be specified using:

### Regular Expressions

Common patterns in FIX protocol:

```
TAG        = [0-9]+
EQUALS     = =
VALUE      = [^|]+
DELIMITER  = \|
```

### Character Classes

```cpp
bool is_tag_char(char c) {
    return c >= '0' && c <= '9';
}

bool is_value_char(char c) {
    return c != '|' && c != '\0';
}

bool is_delimiter(char c) {
    return c == '|' || c == 0x01;  // '|' or SOH
}
```

### Lookahead

Sometimes we need to look ahead to determine token boundaries:

```cpp
// Look ahead to find end of value
size_t find_value_end(std::string_view input, size_t start) {
    size_t pos = start;
    while (pos < input.size() && input[pos] != '|') {
        ++pos;
    }
    return pos;
}
```

---

## Practical Implementation

### Simple Lexer

```cpp
class SimpleLexer {
    std::string_view input;
    size_t pos = 0;
    
public:
    SimpleLexer(std::string_view in) : input(in) {}
    
    Token next() {
        skip_whitespace();
        
        if (pos >= input.size()) {
            return {TokenType::END, "", 0, 0};
        }
        
        char c = input[pos];
        
        // Check for single-character tokens
        if (c == '=') {
            return {TokenType::EQUALS, input.substr(pos++, 1), 0, 0};
        }
        if (c == '|') {
            return {TokenType::DELIMITER, input.substr(pos++, 1), 0, 0};
        }
        
        // Multi-character tokens
        if (isdigit(c)) {
            return read_tag();
        }
        
        return read_value();
    }
    
private:
    Token read_tag() {
        size_t start = pos;
        while (pos < input.size() && isdigit(input[pos])) {
            ++pos;
        }
        return {TokenType::TAG, input.substr(start, pos - start), 0, 0};
    }
    
    Token read_value() {
        size_t start = pos;
        while (pos < input.size() && input[pos] != '|') {
            ++pos;
        }
        return {TokenType::VALUE, input.substr(start, pos - start), 0, 0};
    }
    
    void skip_whitespace() {
        while (pos < input.size() && isspace(input[pos])) {
            ++pos;
        }
    }
};
```

### Usage Example

```cpp
SimpleLexer lexer("55=MSFT|44=123.45|");

Token tok;
while ((tok = lexer.next()).type != TokenType::END) {
    std::cout << "Token: " << to_string(tok.type) 
              << " = '" << tok.lexeme << "'\n";
}

// Output:
// Token: TAG = '55'
// Token: EQUALS = '='
// Token: VALUE = 'MSFT'
// Token: DELIMITER = '|'
// Token: TAG = '44'
// Token: EQUALS = '='
// Token: VALUE = '123.45'
// Token: DELIMITER = '|'
```

---

## FIX Protocol Lexical Analysis

### FIX Message Structure

```
8=FIX.4.4|9=79|35=D|55=MSFT|44=123.45|38=1000|54=1|10=020|
```

### Token Categories

1. **Header Tokens**
   - BeginString (tag 8)
   - BodyLength (tag 9)
   - MsgType (tag 35)

2. **Body Tokens**
   - Application-specific fields
   - Symbol (tag 55)
   - Price (tag 44)
   - Quantity (tag 38)
   - Side (tag 54)

3. **Trailer Tokens**
   - CheckSum (tag 10)

### Lexical Rules

```cpp
// Rule 1: Tags are numeric
bool is_valid_tag(std::string_view lexeme) {
    return !lexeme.empty() && 
           std::all_of(lexeme.begin(), lexeme.end(), ::isdigit);
}

// Rule 2: Values can contain any character except delimiter
bool is_valid_value(std::string_view lexeme) {
    return std::none_of(lexeme.begin(), lexeme.end(), 
                       [](char c) { return c == '|' || c == 0x01; });
}

// Rule 3: Fields are separated by SOH (0x01) or '|'
bool is_delimiter(char c) {
    return c == '|' || c == 0x01;
}
```

### Optimized FIX Lexer

```cpp
class FixLexer {
    std::string_view input;
    size_t pos = 0;
    
public:
    struct Field {
        int tag;
        std::string_view value;
    };
    
    std::optional<Field> next_field() {
        if (pos >= input.size()) {
            return std::nullopt;
        }
        
        // Read tag
        size_t tag_start = pos;
        while (pos < input.size() && isdigit(input[pos])) {
            ++pos;
        }
        
        if (pos >= input.size() || input[pos] != '=') {
            return std::nullopt;  // Invalid format
        }
        
        int tag = parse_int(input.substr(tag_start, pos - tag_start));
        ++pos;  // Skip '='
        
        // Read value
        size_t value_start = pos;
        while (pos < input.size() && !is_delimiter(input[pos])) {
            ++pos;
        }
        
        std::string_view value = input.substr(value_start, pos - value_start);
        
        if (pos < input.size()) {
            ++pos;  // Skip delimiter
        }
        
        return Field{tag, value};
    }
};
```

### Usage in Parser

```cpp
Tick parse_fix_message(std::string_view message) {
    FixLexer lexer(message);
    Tick tick;
    
    while (auto field = lexer.next_field()) {
        switch (field->tag) {
            case 55:  // Symbol
                tick.symbol = field->value;
                break;
            case 44:  // Price
                tick.price = parse_price(field->value);
                break;
            case 38:  // Quantity
                tick.qty = parse_int(field->value);
                break;
            case 54:  // Side
                tick.side = parse_side(field->value);
                break;
        }
    }
    
    return tick;
}
```

---

## Error Handling

### Common Lexical Errors

1. **Invalid Characters**
   ```cpp
   // Input: "55@MSFT|"  (@ instead of =)
   // Error: Unexpected character '@' at position 2
   ```

2. **Incomplete Tokens**
   ```cpp
   // Input: "55=MSFT"  (missing delimiter)
   // Error: Unexpected end of input
   ```

3. **Invalid Tag Format**
   ```cpp
   // Input: "ABC=value|"  (non-numeric tag)
   // Error: Invalid tag format 'ABC'
   ```

### Error Recovery

```cpp
class RobustLexer {
    std::string_view input;
    size_t pos = 0;
    std::vector<std::string> errors;
    
public:
    Token next() {
        try {
            return next_token_impl();
        } catch (const LexicalError& e) {
            errors.push_back(e.what());
            recover();
            return next();  // Try again after recovery
        }
    }
    
private:
    void recover() {
        // Skip to next delimiter
        while (pos < input.size() && !is_delimiter(input[pos])) {
            ++pos;
        }
        if (pos < input.size()) {
            ++pos;  // Skip delimiter
        }
    }
};
```

---

## Performance Considerations

### Optimization Techniques

1. **Minimize Allocations**
   ```cpp
   // ❌ Bad: Allocates for each token
   std::string lexeme = input.substr(start, length);
   
   // ✅ Good: Zero-copy reference
   std::string_view lexeme = input.substr(start, length);
   ```

2. **Inline Character Tests**
   ```cpp
   // ❌ Bad: Function call overhead
   if (isdigit(c)) { ... }
   
   // ✅ Good: Inline comparison
   if (c >= '0' && c <= '9') { ... }
   ```

3. **Batch Processing**
   ```cpp
   // Process multiple characters at once
   const char* ptr = input.data();
   const char* end = ptr + input.size();
   
   while (ptr < end && *ptr >= '0' && *ptr <= '9') {
       ++ptr;
   }
   ```

### Benchmark Results

From our FIX parser implementation:

| Approach | Throughput | Notes |
|----------|-----------|-------|
| Regex-based | ~50K msgs/sec | Flexible but slow |
| Character-by-character | ~115K msgs/sec | Simple but allocates |
| Zero-copy lexer | ~473K msgs/sec | Fast, no allocations |
| Optimized FSM | ~581K msgs/sec | Fastest, most complex |

---

## Summary

Lexical analysis is the foundation of parsing:

1. **Converts characters to tokens** - Makes parsing easier
2. **Uses FSMs** - Systematic state-based processing
3. **Pattern matching** - Identifies token boundaries
4. **Error handling** - Detects and reports issues
5. **Performance critical** - First bottleneck in parsing

Our FIX parser demonstrates these principles:
- Zero-copy tokenization with `string_view`
- Implicit FSM in character-by-character parsing
- Optimized for high-frequency trading (473K+ msgs/sec)

---

## Further Reading

- [Compilers: Principles, Techniques, and Tools](https://en.wikipedia.org/wiki/Compilers:_Principles,_Techniques,_and_Tools) (Dragon Book)
- [Flex - Fast Lexical Analyzer](https://github.com/westes/flex)
- [Regular Expressions in C++](https://en.cppreference.com/w/cpp/regex)
- [Finite State Machines](https://en.wikipedia.org/wiki/Finite-state_machine)

## Related Documents

- [Zero-Copy Parsing Guide](zero_copy_parsing_guide.md)
- [FIX Protocol Reference](fix_protocol_reference.md)
- [FSM Parser Design](fsm_diagram.md) (Week 3)