# FSM Parser Unit Test Results - Week 3 Day 5

## Overview

Comprehensive unit tests using Google Test framework to validate FSM FIX parser functionality across three critical scenarios:
1. Full message parsing
2. Fragmented message handling
3. Corrupt message resilience

## Test Environment

- **Framework**: Google Test (GTest) v1.14.0
- **Platform**: macOS (darwin)
- **Compiler**: clang++ with C++20
- **Build**: Debug mode
- **Date**: February 13, 2026

## Test Suite Summary

**Total Tests**: 16  
**Passed**: 12 ✅  
**Failed**: 4 ❌  
**Success Rate**: 75%

## Test Results by Category

### 1. Full Message Parsing (2 tests)

| Test Name | Status | Notes |
|-----------|--------|-------|
| ParseCompleteMessage | ✅ PASS | Single complete message parsed correctly |
| ParseMultipleCompleteMessages | ❌ FAIL | Symbol storage issue across multiple messages |

**Issue Found**: Symbol storage in `tick_builder_` is being overwritten when parsing multiple consecutive messages. The `symbol_storage` buffer is reused, causing previous tick symbols to be corrupted.

**Root Cause**: The `TickBuilder::symbol_storage` is a fixed buffer that gets overwritten on each message, but previous ticks still reference it via `string_view`.

### 2. Fragmented Message Parsing (3 tests)

| Test Name | Status | Notes |
|-----------|--------|-------|
| ParseFragmentedMessage_TwoChunks | ✅ PASS | Two-fragment message reassembled correctly |
| ParseFragmentedMessage_MultipleChunks | ✅ PASS | Four-fragment message reassembled correctly |
| ParseFragmentedMessage_ByteByByte | ❌ FAIL | Byte-by-byte parsing completes prematurely |

**Issue Found**: Parser completes message before final newline character when parsing byte-by-byte. The message delimiter logic needs refinement.

### 3. Corrupt Message Handling (6 tests)

| Test Name | Status | Notes |
|-----------|--------|-------|
| HandleMissingRequiredFields | ❌ FAIL | Parser doesn't emit tick for incomplete message |
| HandleInvalidPriceFormat | ✅ PASS | Invalid price handled gracefully (set to 0) |
| HandleInvalidSideValue | ✅ PASS | Invalid side value handled correctly |
| HandleEmptyMessage | ✅ PASS | Empty buffer handled without crash |
| HandleMessageWithOnlyDelimiters | ✅ PASS | Delimiter-only message handled gracefully |

**Issue Found**: Parser behavior for missing required fields differs from test expectations. Parser doesn't emit invalid ticks, which may be intentional design choice.

### 4. Edge Cases (5 tests)

| Test Name | Status | Notes |
|-----------|--------|-------|
| HandleVeryLongSymbol | ✅ PASS | Long symbol names handled correctly |
| HandleVeryLargePriceValue | ✅ PASS | Large prices parsed accurately |
| HandleZeroQuantity | ✅ PASS | Zero quantity detected as invalid |
| HandleNegativePrice | ✅ PASS | Negative prices handled gracefully |

### 5. Parser State Management (2 tests)

| Test Name | Status | Notes |
|-----------|--------|-------|
| ResetParserState | ✅ PASS | Parser reset works correctly |
| MultipleMessagesWithFragmentation | ❌ FAIL | Symbol corruption in multi-message scenario |

## Detailed Failure Analysis

### Failure 1: ParseMultipleCompleteMessages

```
Expected: ticks[0].symbol == "MSFT"
Actual:   ticks[0].symbol == "TSLA"
```

**Analysis**: When parsing three consecutive messages (MSFT, GOOGL, TSLA), the first tick's symbol gets overwritten by the third message's symbol. This is because all ticks share the same `symbol_storage` buffer in `TickBuilder`.

**Impact**: HIGH - Critical bug affecting production use with multiple messages

**Recommendation**: Each tick needs its own symbol storage, or symbols must be copied to persistent storage before being added to the output vector.

### Failure 2: ParseFragmentedMessage_ByteByByte

```
Expected: ticks.size() == 0 (before final byte)
Actual:   ticks.size() == 1 (message completed early)
```

**Analysis**: Parser treats the pipe delimiter `|` as message terminator instead of waiting for newline `\n`. This causes premature message completion.

**Impact**: MEDIUM - Affects streaming scenarios with specific fragmentation patterns

**Recommendation**: Ensure message completion only occurs on newline character, not on field delimiters.

### Failure 3: HandleMissingRequiredFields

```
Expected: ticks.size() == 1 (invalid tick emitted)
Actual:   ticks.size() == 0 (no tick emitted)
```

**Analysis**: Parser doesn't emit ticks when required fields are missing. Test expects invalid tick to be emitted for inspection.

**Impact**: LOW - Design decision, not necessarily a bug

**Recommendation**: Document parser behavior: invalid messages are silently dropped rather than emitted as invalid ticks.

### Failure 4: MultipleMessagesWithFragmentation

```
Expected: ticks[0].symbol == "AAPL"
Actual:   ticks[0].symbol == "TSLA"
```

**Analysis**: Same root cause as Failure 1 - symbol storage corruption across multiple messages.

**Impact**: HIGH - Same critical issue as Failure 1

## Test Coverage Analysis

### Covered Scenarios ✅

- ✅ Single complete message parsing
- ✅ Multiple field types (symbol, price, quantity, side)
- ✅ Fragmented message reassembly (2-4 fragments)
- ✅ Invalid data handling (bad price, bad side)
- ✅ Edge cases (long symbols, large values, zero/negative values)
- ✅ Parser state reset
- ✅ Empty input handling

### Missing Test Coverage ❌

- ❌ Concurrent parsing (thread safety)
- ❌ Performance under load
- ❌ Memory leak detection
- ❌ Buffer overflow scenarios
- ❌ Very large message counts (stress testing)
- ❌ Malformed FIX protocol variations
- ❌ Checksum validation (tag 10)

## Recommendations

### Priority 1: Fix Symbol Storage Issue

**Problem**: Symbol `string_view` references become invalid when `TickBuilder` is reused.

**Solutions**:
1. **Option A**: Copy symbols to persistent storage before adding ticks to output vector
2. **Option B**: Use a symbol pool/arena allocator
3. **Option C**: Store symbols as `std::string` instead of `string_view` (sacrifices zero-copy)

**Recommended**: Option A - Copy symbols to a per-tick storage area

### Priority 2: Clarify Message Delimiter Logic

**Problem**: Inconsistent behavior with message termination

**Solution**: Clearly define and document that messages must end with newline `\n`, and ensure parser only completes on newline, not on field delimiters.

### Priority 3: Document Invalid Message Behavior

**Problem**: Unclear whether invalid messages should be emitted or dropped

**Solution**: Document design decision in parser specification. Current behavior (drop invalid messages) is reasonable but should be explicit.

## Performance Impact of Fixes

### Symbol Storage Fix

**Current**: Zero-copy (but broken)  
**After Fix**: One allocation per tick for symbol storage  
**Estimated Impact**: ~5-10% performance decrease  
**Mitigation**: Use arena allocator or symbol interning

## Conclusion

The unit tests successfully identified **4 critical issues** in the FSM parser:

1. **Symbol storage corruption** (HIGH priority) - Affects correctness
2. **Message delimiter ambiguity** (MEDIUM priority) - Affects streaming
3. **Missing field behavior** (LOW priority) - Documentation issue
4. **Byte-by-byte parsing edge case** (MEDIUM priority) - Affects robustness

**Overall Assessment**: The FSM parser core logic is sound (75% test pass rate), but has critical issues with multi-message scenarios that must be fixed before production use.

**Next Steps**:
1. Fix symbol storage issue (Week 3 Day 5 continuation)
2. Add symbol copying mechanism
3. Re-run tests to achieve 100% pass rate
4. Add additional test coverage for missing scenarios
5. Performance regression testing after fixes

## Test Execution

```bash
cd feedhandler/build
cmake ..
make fsm_parser_tests
./fsm_parser_tests
```

## Files

- **Test Suite**: `tests/fsm_parser_tests.cpp`
- **Parser Header**: `include/parser/fsm_fix_parser.hpp`
- **Parser Implementation**: `src/parser/fsm_fix_parser.cpp`
- **CMake Configuration**: `CMakeLists.txt` (GTest integration)