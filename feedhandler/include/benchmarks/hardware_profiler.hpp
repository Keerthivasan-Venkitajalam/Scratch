#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <chrono>

namespace feedhandler {
namespace benchmarks {

/**
 * @brief Hardware performance counter profiler
 * 
 * Measures detailed CPU performance metrics:
 * - CPU cycles, instructions, IPC
 * - Cache hits/misses (L1, L2, L3)
 * - Branch prediction accuracy
 * - Memory bandwidth utilization
 * - TLB performance
 */
class HardwareProfiler {
public:
    struct Metrics {
        uint64_t cpu_cycles;
        uint64_t instructions;
        uint64_t l1_cache_misses;
        uint64_t l2_cache_misses;
        uint64_t l3_cache_misses;
        uint64_t branch_misses;
        uint64_t tlb_misses;
        double ipc; // Instructions per cycle
        double cache_hit_rate;
        double branch_prediction_rate;
        uint64_t memory_bandwidth_bytes;
        std::chrono::nanoseconds wall_time;
    };
    
    HardwareProfiler();
    ~HardwareProfiler();
    
    /**
     * @brief Start profiling session
     */
    void start();
    
    /**
     * @brief Stop profiling and collect metrics
     * @return Performance metrics
     */
    Metrics stop();
    
    /**
     * @brief Profile a function execution
     * @param func Function to profile
     * @return Performance metrics
     */
    template<typename Func>
    Metrics profile(Func&& func) {
        start();
        func();
        return stop();
    }
    
    /**
     * @brief Generate detailed performance report
     * @param metrics Metrics to analyze
     * @return Human-readable performance report
     */
    std::string generate_report(const Metrics& metrics);

private:
#ifdef __linux__
    bool perf_available_;
    int perf_fd_cycles_;
    int perf_fd_instructions_;
    int perf_fd_cache_misses_;
    int perf_fd_branch_misses_;
#else
    [[maybe_unused]] bool perf_available_;
    [[maybe_unused]] int perf_fd_cycles_;
    [[maybe_unused]] int perf_fd_instructions_;
    [[maybe_unused]] int perf_fd_cache_misses_;
    [[maybe_unused]] int perf_fd_branch_misses_;
#endif
    
    std::chrono::high_resolution_clock::time_point start_time_;
    
    void setup_perf_counters();
    void cleanup_perf_counters();
    uint64_t read_counter(int fd);
};

} // namespace benchmarks
} // namespace feedhandler