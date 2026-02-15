#!/bin/bash

# Performance profiling script for FeedHandler FSM parser
# Measures branch prediction, cache performance, and instruction throughput

set -e

BINARY="./build/feedhandler"
BENCHMARK_BINARY="./build/parser_benchmark"

echo "=========================================="
echo "FeedHandler Performance Profiling"
echo "=========================================="
echo ""

# Check if binary exists
if [ ! -f "$BENCHMARK_BINARY" ]; then
    echo "Error: Benchmark binary not found at $BENCHMARK_BINARY"
    echo "Please build the project first:"
    echo "  cd build && cmake .. && make"
    exit 1
fi

# Check if perf is available
if ! command -v perf &> /dev/null; then
    echo "Warning: 'perf' command not found. Install with:"
    echo "  Linux: sudo apt-get install linux-tools-generic"
    echo "  macOS: perf is not available, use Instruments instead"
    echo ""
    echo "Running benchmark without perf profiling..."
    $BENCHMARK_BINARY
    exit 0
fi

echo "1. Basic Performance Counters"
echo "------------------------------"
perf stat -e cycles,instructions,branches,branch-misses,cache-references,cache-misses \
    $BENCHMARK_BINARY 2>&1 | grep -E "(cycles|instructions|branches|cache)"

echo ""
echo "2. Branch Prediction Analysis"
echo "------------------------------"
perf stat -e branches,branch-misses,branch-loads,branch-load-misses \
    $BENCHMARK_BINARY 2>&1 | grep -E "branch"

echo ""
echo "3. Cache Performance"
echo "------------------------------"
perf stat -e L1-dcache-loads,L1-dcache-load-misses,L1-icache-load-misses \
    $BENCHMARK_BINARY 2>&1 | grep -E "cache"

echo ""
echo "4. Instructions Per Cycle (IPC)"
echo "------------------------------"
perf stat -e cycles,instructions \
    $BENCHMARK_BINARY 2>&1 | grep -E "(cycles|instructions|insn per cycle)"

echo ""
echo "=========================================="
echo "Profiling Complete"
echo "=========================================="
echo ""
echo "To generate detailed profile:"
echo "  perf record -g $BENCHMARK_BINARY"
echo "  perf report"
echo ""
echo "To analyze branch misses:"
echo "  perf record -e branch-misses:u -g $BENCHMARK_BINARY"
echo "  perf report --sort symbol"
