#pragma once

#include <string>
#include <unordered_map>
#include <memory>

namespace feedhandler {
namespace config {

/**
 * @brief Production-grade performance configuration system
 * 
 * Allows runtime tuning of all performance parameters:
 * - CPU affinity and thread pinning
 * - Memory allocation strategies
 * - SIMD instruction set selection
 * - Queue sizes and batching parameters
 * - Profiling and monitoring settings
 */
class PerformanceConfig {
public:
    struct CPUConfig {
        std::vector<int> parser_thread_affinity;
        std::vector<int> network_thread_affinity;
        bool enable_hyperthreading = false;
        int numa_node = -1; // -1 = auto-detect
        bool enable_cpu_isolation = true;
    };
    
    struct MemoryConfig {
        size_t zero_latency_pool_size = 1024 * 1024 * 1024; // 1GB
        bool enable_huge_pages = true;
        bool enable_numa_allocation = true;
        size_t tick_pool_size = 10000;
        bool prefault_memory = true;
    };
    
    struct ParserConfig {
        bool enable_simd = true;
        std::string simd_instruction_set = "AVX2"; // SSE4.2, AVX2, AVX512
        bool enable_branch_prediction = true;
        bool enable_garbage_recovery = true;
        size_t batch_size = 32;
    };
    
    struct QueueConfig {
        size_t ring_buffer_size = 1024;
        bool enable_batching = true;
        size_t batch_size = 16;
        bool enable_backpressure = false;
    };
    
    struct MonitoringConfig {
        bool enable_hardware_counters = true;
        bool enable_latency_tracking = true;
        bool enable_memory_profiling = false;
        int metrics_update_interval_ms = 1000;
        std::string metrics_output_file = "performance_metrics.json";
    };
    
    static PerformanceConfig& instance();
    
    /**
     * @brief Load configuration from file
     * @param config_file Path to JSON configuration file
     * @return true if successful
     */
    bool load_from_file(const std::string& config_file);
    
    /**
     * @brief Save current configuration to file
     * @param config_file Path to output file
     * @return true if successful
     */
    bool save_to_file(const std::string& config_file) const;
    
    /**
     * @brief Auto-tune configuration based on hardware
     * Detects CPU capabilities, memory topology, etc.
     */
    void auto_tune();
    
    // Configuration accessors
    const CPUConfig& cpu() const { return cpu_config_; }
    const MemoryConfig& memory() const { return memory_config_; }
    const ParserConfig& parser() const { return parser_config_; }
    const QueueConfig& queue() const { return queue_config_; }
    const MonitoringConfig& monitoring() const { return monitoring_config_; }
    
    // Configuration mutators
    CPUConfig& cpu() { return cpu_config_; }
    MemoryConfig& memory() { return memory_config_; }
    ParserConfig& parser() { return parser_config_; }
    QueueConfig& queue() { return queue_config_; }
    MonitoringConfig& monitoring() { return monitoring_config_; }

private:
    PerformanceConfig() = default;
    
    CPUConfig cpu_config_;
    MemoryConfig memory_config_;
    ParserConfig parser_config_;
    QueueConfig queue_config_;
    MonitoringConfig monitoring_config_;
    
    void detect_hardware_capabilities();
    void optimize_for_latency();
    void optimize_for_throughput();
};

} // namespace config
} // namespace feedhandler