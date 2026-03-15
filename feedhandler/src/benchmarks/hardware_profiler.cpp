#include "benchmarks/hardware_profiler.hpp"
#include <sstream>
#include <iomanip>

#ifdef __linux__
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#endif

namespace feedhandler {
namespace benchmarks {

HardwareProfiler::HardwareProfiler() 
    : perf_available_(false), perf_fd_cycles_(-1), perf_fd_instructions_(-1),
      perf_fd_cache_misses_(-1), perf_fd_branch_misses_(-1) {
#ifdef __linux__
    setup_perf_counters();
#endif
}

HardwareProfiler::~HardwareProfiler() {
#ifdef __linux__
    cleanup_perf_counters();
#endif
}

void HardwareProfiler::setup_perf_counters() {
#ifdef __linux__
    struct perf_event_attr pe;
    
    // Setup CPU cycles counter
    std::memset(&pe, 0, sizeof(pe));
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(pe);
    pe.config = PERF_COUNT_HW_CPU_CYCLES;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    
    perf_fd_cycles_ = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
    
    // Setup instructions counter
    pe.config = PERF_COUNT_HW_INSTRUCTIONS;
    perf_fd_instructions_ = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
    
    // Setup cache misses counter
    pe.config = PERF_COUNT_HW_CACHE_MISSES;
    perf_fd_cache_misses_ = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
    
    // Setup branch misses counter
    pe.config = PERF_COUNT_HW_BRANCH_MISSES;
    perf_fd_branch_misses_ = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
    
    perf_available_ = (perf_fd_cycles_ >= 0 && perf_fd_instructions_ >= 0);
#endif
}

void HardwareProfiler::cleanup_perf_counters() {
#ifdef __linux__
    if (perf_fd_cycles_ >= 0) close(perf_fd_cycles_);
    if (perf_fd_instructions_ >= 0) close(perf_fd_instructions_);
    if (perf_fd_cache_misses_ >= 0) close(perf_fd_cache_misses_);
    if (perf_fd_branch_misses_ >= 0) close(perf_fd_branch_misses_);
#endif
}

void HardwareProfiler::start() {
    start_time_ = std::chrono::high_resolution_clock::now();
    
#ifdef __linux__
    if (!perf_available_) return;
    
    // Reset and enable counters
    ioctl(perf_fd_cycles_, PERF_EVENT_IOC_RESET, 0);
    ioctl(perf_fd_instructions_, PERF_EVENT_IOC_RESET, 0);
    ioctl(perf_fd_cache_misses_, PERF_EVENT_IOC_RESET, 0);
    ioctl(perf_fd_branch_misses_, PERF_EVENT_IOC_RESET, 0);
    
    ioctl(perf_fd_cycles_, PERF_EVENT_IOC_ENABLE, 0);
    ioctl(perf_fd_instructions_, PERF_EVENT_IOC_ENABLE, 0);
    ioctl(perf_fd_cache_misses_, PERF_EVENT_IOC_ENABLE, 0);
    ioctl(perf_fd_branch_misses_, PERF_EVENT_IOC_ENABLE, 0);
#endif
}

HardwareProfiler::Metrics HardwareProfiler::stop() {
    auto end_time = std::chrono::high_resolution_clock::now();
    
    Metrics metrics;
    metrics.wall_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end_time - start_time_);
    
#ifdef __linux__
    if (!perf_available_) {
        return metrics; // Return metrics with only wall time
    }
    
    // Disable counters
    ioctl(perf_fd_cycles_, PERF_EVENT_IOC_DISABLE, 0);
    ioctl(perf_fd_instructions_, PERF_EVENT_IOC_DISABLE, 0);
    ioctl(perf_fd_cache_misses_, PERF_EVENT_IOC_DISABLE, 0);
    ioctl(perf_fd_branch_misses_, PERF_EVENT_IOC_DISABLE, 0);
    
    // Read counter values
    metrics.cpu_cycles = read_counter(perf_fd_cycles_);
    metrics.instructions = read_counter(perf_fd_instructions_);
    metrics.l1_cache_misses = read_counter(perf_fd_cache_misses_);
    metrics.branch_misses = read_counter(perf_fd_branch_misses_);
    
    // Calculate derived metrics
    metrics.ipc = metrics.cpu_cycles > 0 ? 
        static_cast<double>(metrics.instructions) / metrics.cpu_cycles : 0.0;
    
    metrics.cache_hit_rate = metrics.instructions > 0 ?
        1.0 - (static_cast<double>(metrics.l1_cache_misses) / metrics.instructions) : 0.0;
    
    metrics.branch_prediction_rate = metrics.instructions > 0 ?
        1.0 - (static_cast<double>(metrics.branch_misses) / metrics.instructions) : 0.0;
#endif
    
    return metrics;
}

uint64_t HardwareProfiler::read_counter(int fd) {
#ifdef __linux__
    if (fd < 0) return 0;
    
    uint64_t value;
    ssize_t bytes_read = read(fd, &value, sizeof(value));
    return (bytes_read == sizeof(value)) ? value : 0;
#else
    (void)fd; // Suppress unused parameter warning
    return 0;
#endif
}

std::string HardwareProfiler::generate_report(const Metrics& metrics) {
    std::ostringstream report;
    
    report << "=== Hardware Performance Report ===\n";
    report << std::fixed << std::setprecision(2);
    
    report << "Wall Time: " << metrics.wall_time.count() << " ns\n";
    
#ifdef __linux__
    if (perf_available_) {
        report << "CPU Cycles: " << metrics.cpu_cycles << "\n";
        report << "Instructions: " << metrics.instructions << "\n";
        report << "IPC (Instructions per Cycle): " << metrics.ipc << "\n";
        
        report << "\nCache Performance:\n";
        report << "L1 Cache Misses: " << metrics.l1_cache_misses << "\n";
        report << "Cache Hit Rate: " << (metrics.cache_hit_rate * 100) << "%\n";
        
        report << "\nBranch Prediction:\n";
        report << "Branch Misses: " << metrics.branch_misses << "\n";
        report << "Branch Prediction Rate: " << (metrics.branch_prediction_rate * 100) << "%\n";
        
        // Performance analysis
        report << "\nPerformance Analysis:\n";
        if (metrics.ipc < 1.0) {
            report << "- Low IPC suggests CPU stalls (memory/cache issues)\n";
        } else if (metrics.ipc > 2.0) {
            report << "- High IPC indicates good CPU utilization\n";
        }
        
        if (metrics.cache_hit_rate < 0.95) {
            report << "- Cache hit rate below 95% - consider data locality optimization\n";
        }
        
        if (metrics.branch_prediction_rate < 0.90) {
            report << "- Branch prediction rate below 90% - consider branch optimization\n";
        }
    } else {
        report << "\nNote: Hardware performance counters not available on this platform.\n";
        report << "Only wall time measurement is provided.\n";
    }
#else
    report << "\nNote: Hardware performance counters not available on macOS.\n";
    report << "Only wall time measurement is provided.\n";
#endif
    
    return report.str();
}

} // namespace benchmarks
} // namespace feedhandler