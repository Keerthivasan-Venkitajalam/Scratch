// Google Benchmark suite for FIX parser comparison
// Compares: Naive, StringView, and FSM parsers
// Goal: FSM should be ≥5× faster than naive

#include <benchmark/benchmark.h>
#include "parser/naive_fix_parser.hpp"
#include "parser/stringview_fix_parser.hpp"
#include "parser/fsm_fix_parser.hpp"
#include "common/tick.hpp"

#include <string>
#include <vector>

using namespace feedhandler;

// Sample FIX messages for benchmarking
static const std::string SIMPLE_MESSAGE = 
    "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.2500|38=500|54=1|52=20240131-12:34:56|10=020|\n";

static const std::string COMPLEX_MESSAGE = 
    "8=FIX.4.4|9=120|35=D|49=SENDER|56=TARGET|34=1|52=20240131-12:34:56.789|"
    "55=MSFT|54=2|38=1000|44=380.7500|40=2|59=0|21=1|207=NASDAQ|10=123|\n";

static const std::string MULTIPLE_MESSAGES = 
    "8=FIX.4.4|9=79|35=D|55=AAPL|44=150.25|38=500|54=1|52=20240131-12:34:56|10=020|\n"
    "8=FIX.4.4|9=79|35=D|55=GOOGL|44=2800.50|38=100|54=2|52=20240131-12:34:57|10=021|\n"
    "8=FIX.4.4|9=79|35=D|55=TSLA|44=245.75|38=750|54=1|52=20240131-12:34:58|10=022|\n";

// ============================================================================
// Naive Parser Benchmarks
// ============================================================================

static void BM_NaiveParser_SingleMessage(benchmark::State& state) {
    for (auto _ : state) {
        common::Tick tick = parser::NaiveFixParser::parse_message(SIMPLE_MESSAGE);
        benchmark::DoNotOptimize(tick);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * SIMPLE_MESSAGE.size());
}
BENCHMARK(BM_NaiveParser_SingleMessage);

static void BM_NaiveParser_ComplexMessage(benchmark::State& state) {
    for (auto _ : state) {
        common::Tick tick = parser::NaiveFixParser::parse_message(COMPLEX_MESSAGE);
        benchmark::DoNotOptimize(tick);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * COMPLEX_MESSAGE.size());
}
BENCHMARK(BM_NaiveParser_ComplexMessage);

static void BM_NaiveParser_Batch(benchmark::State& state) {
    const int batch_size = state.range(0);
    
    // Create batch of messages
    std::string batch;
    batch.reserve(SIMPLE_MESSAGE.size() * batch_size);
    for (int i = 0; i < batch_size; ++i) {
        batch += SIMPLE_MESSAGE;
    }
    
    for (auto _ : state) {
        // Parse each message in batch
        size_t pos = 0;
        while (pos < batch.size()) {
            size_t end = batch.find('\n', pos);
            if (end == std::string::npos) break;
            
            std::string msg = batch.substr(pos, end - pos + 1);
            common::Tick tick = parser::NaiveFixParser::parse_message(msg);
            benchmark::DoNotOptimize(tick);
            
            pos = end + 1;
        }
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.SetBytesProcessed(state.iterations() * batch.size());
}
BENCHMARK(BM_NaiveParser_Batch)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// StringView Parser Benchmarks
// ============================================================================

static void BM_StringViewParser_SingleMessage(benchmark::State& state) {
    for (auto _ : state) {
        common::Tick tick = parser::StringViewFixParser::parse_message(SIMPLE_MESSAGE);
        benchmark::DoNotOptimize(tick);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * SIMPLE_MESSAGE.size());
}
BENCHMARK(BM_StringViewParser_SingleMessage);

static void BM_StringViewParser_ComplexMessage(benchmark::State& state) {
    for (auto _ : state) {
        common::Tick tick = parser::StringViewFixParser::parse_message(COMPLEX_MESSAGE);
        benchmark::DoNotOptimize(tick);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * COMPLEX_MESSAGE.size());
}
BENCHMARK(BM_StringViewParser_ComplexMessage);

static void BM_StringViewParser_Batch(benchmark::State& state) {
    const int batch_size = state.range(0);
    
    std::string batch;
    batch.reserve(SIMPLE_MESSAGE.size() * batch_size);
    for (int i = 0; i < batch_size; ++i) {
        batch += SIMPLE_MESSAGE;
    }
    
    for (auto _ : state) {
        size_t pos = 0;
        while (pos < batch.size()) {
            size_t end = batch.find('\n', pos);
            if (end == std::string::npos) break;
            
            std::string_view msg(batch.data() + pos, end - pos + 1);
            common::Tick tick = parser::StringViewFixParser::parse_message(msg);
            benchmark::DoNotOptimize(tick);
            
            pos = end + 1;
        }
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.SetBytesProcessed(state.iterations() * batch.size());
}
BENCHMARK(BM_StringViewParser_Batch)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// FSM Parser Benchmarks
// ============================================================================

static void BM_FSMParser_SingleMessage(benchmark::State& state) {
    parser::FSMFixParser parser;
    std::vector<common::Tick> ticks;
    ticks.reserve(1);
    
    for (auto _ : state) {
        ticks.clear();
        parser.reset();
        parser.parse(SIMPLE_MESSAGE.data(), SIMPLE_MESSAGE.size(), ticks);
        benchmark::DoNotOptimize(ticks);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * SIMPLE_MESSAGE.size());
}
BENCHMARK(BM_FSMParser_SingleMessage);

static void BM_FSMParser_ComplexMessage(benchmark::State& state) {
    parser::FSMFixParser parser;
    std::vector<common::Tick> ticks;
    ticks.reserve(1);
    
    for (auto _ : state) {
        ticks.clear();
        parser.reset();
        parser.parse(COMPLEX_MESSAGE.data(), COMPLEX_MESSAGE.size(), ticks);
        benchmark::DoNotOptimize(ticks);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * COMPLEX_MESSAGE.size());
}
BENCHMARK(BM_FSMParser_ComplexMessage);

static void BM_FSMParser_Batch(benchmark::State& state) {
    parser::FSMFixParser parser;
    const int batch_size = state.range(0);
    
    std::string batch;
    batch.reserve(SIMPLE_MESSAGE.size() * batch_size);
    for (int i = 0; i < batch_size; ++i) {
        batch += SIMPLE_MESSAGE;
    }
    
    std::vector<common::Tick> ticks;
    ticks.reserve(batch_size);
    
    for (auto _ : state) {
        ticks.clear();
        parser.reset();
        parser.parse(batch.data(), batch.size(), ticks);
        benchmark::DoNotOptimize(ticks);
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.SetBytesProcessed(state.iterations() * batch.size());
}
BENCHMARK(BM_FSMParser_Batch)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// Streaming/Fragmentation Benchmarks (FSM only)
// ============================================================================

static void BM_FSMParser_Fragmented(benchmark::State& state) {
    parser::FSMFixParser parser;
    const int chunk_size = state.range(0);
    std::vector<common::Tick> ticks;
    ticks.reserve(10);
    
    for (auto _ : state) {
        ticks.clear();
        parser.reset();
        
        // Simulate fragmented TCP stream
        size_t offset = 0;
        while (offset < MULTIPLE_MESSAGES.size()) {
            size_t len = std::min<size_t>(chunk_size, MULTIPLE_MESSAGES.size() - offset);
            parser.parse(MULTIPLE_MESSAGES.data() + offset, len, ticks);
            offset += len;
        }
        
        benchmark::DoNotOptimize(ticks);
    }
    
    state.SetItemsProcessed(state.iterations() * 3); // 3 messages
    state.SetBytesProcessed(state.iterations() * MULTIPLE_MESSAGES.size());
}
BENCHMARK(BM_FSMParser_Fragmented)->Arg(8)->Arg(16)->Arg(32)->Arg(64);

// ============================================================================
// Memory Allocation Benchmarks
// ============================================================================

static void BM_NaiveParser_Allocations(benchmark::State& state) {
    for (auto _ : state) {
        // Naive parser allocates strings internally
        common::Tick tick = parser::NaiveFixParser::parse_message(SIMPLE_MESSAGE);
        benchmark::DoNotOptimize(tick);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_NaiveParser_Allocations);

static void BM_FSMParser_NoAllocations(benchmark::State& state) {
    parser::FSMFixParser parser;
    std::vector<common::Tick> ticks;
    ticks.reserve(1);
    
    for (auto _ : state) {
        ticks.clear();
        parser.reset();
        // FSM parser should not allocate in hot path
        parser.parse(SIMPLE_MESSAGE.data(), SIMPLE_MESSAGE.size(), ticks);
        benchmark::DoNotOptimize(ticks);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_FSMParser_NoAllocations);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
