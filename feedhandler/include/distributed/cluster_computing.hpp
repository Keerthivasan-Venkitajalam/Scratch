#pragma once

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <atomic>
#include <future>

namespace feedhandler {
namespace distributed {

/**
 * @brief Distributed computing framework for massive parallel processing
 * 
 * This framework enables horizontal scaling across multiple machines
 * for processing enormous volumes of market data and running complex
 * analytics across distributed clusters.
 * 
 * Features:
 * - Automatic load balancing
 * - Fault tolerance and recovery
 * - Dynamic scaling
 * - Distributed caching
 * - Cross-datacenter replication
 */
class ClusterManager {
public:
    struct NodeInfo {
        std::string node_id;
        std::string ip_address;
        uint16_t port;
        double cpu_utilization;
        size_t memory_usage_mb;
        size_t network_bandwidth_mbps;
        bool is_healthy;
        uint64_t last_heartbeat;
        std::vector<std::string> assigned_symbols;
    };
    
    struct ClusterConfig {
        std::string cluster_name = "trading_cluster";
        std::vector<std::string> seed_nodes;
        uint16_t base_port = 8000;
        size_t replication_factor = 3;
        double load_balance_threshold = 0.8;
        uint32_t heartbeat_interval_ms = 1000;
        bool enable_auto_scaling = true;
        size_t max_nodes = 100;
    };
    
    ClusterManager(const ClusterConfig& config);
    ~ClusterManager();
    
    /**
     * @brief Join cluster as a new node
     * @param node_capabilities Node resource capabilities
     * @return Success status and assigned node ID
     */
    std::pair<bool, std::string> join_cluster(const NodeInfo& node_capabilities);
    
    /**
     * @brief Leave cluster gracefully
     * @param node_id Node to remove
     */
    void leave_cluster(const std::string& node_id);
    
    /**
     * @brief Distribute workload across cluster nodes
     * @param symbols List of symbols to process
     * @param processing_function Function to execute on each node
     * @return Distribution mapping
     */
    std::unordered_map<std::string, std::vector<std::string>> 
    distribute_workload(const std::vector<std::string>& symbols,
                       const std::function<void(const std::vector<std::string>&)>& processing_function);
    
    /**
     * @brief Execute distributed computation
     * @param task_name Unique task identifier
     * @param input_data Input data to process
     * @param map_function Map phase function
     * @param reduce_function Reduce phase function
     * @return Computation result
     */
    template<typename InputType, typename IntermediateType, typename OutputType>
    std::future<OutputType> execute_map_reduce(
        const std::string& task_name,
        const std::vector<InputType>& input_data,
        std::function<std::vector<IntermediateType>(const InputType&)> map_function,
        std::function<OutputType(const std::vector<IntermediateType>&)> reduce_function);
    
    /**
     * @brief Get cluster status and health
     */
    struct ClusterStatus {
        size_t total_nodes;
        size_t healthy_nodes;
        double average_cpu_utilization;
        size_t total_memory_mb;
        double network_throughput_gbps;
        std::vector<NodeInfo> node_details;
    };
    
    ClusterStatus get_cluster_status() const;
    
    /**
     * @brief Enable automatic failover for high availability
     * @param enable Enable/disable automatic failover
     */
    void set_auto_failover(bool enable);

private:
    ClusterConfig config_;
    std::string local_node_id_;
    std::unordered_map<std::string, NodeInfo> cluster_nodes_;
    
    std::atomic<bool> running_;
    std::thread heartbeat_thread_;
    std::thread load_balancer_thread_;
    
    void heartbeat_loop();
    void load_balance_loop();
    void handle_node_failure(const std::string& node_id);
    void redistribute_workload();
    
    // Network communication
    bool send_message(const std::string& node_id, const std::string& message);
    void broadcast_message(const std::string& message);
    
    // Load balancing algorithms
    std::vector<std::string> select_optimal_nodes(size_t required_nodes);
    double calculate_node_load(const NodeInfo& node);
};

/**
 * @brief Distributed cache for market data
 */
class DistributedCache {
public:
    struct CacheConfig {
        size_t max_memory_mb = 1024;        // 1GB per node
        uint32_t ttl_seconds = 3600;        // 1 hour TTL
        size_t replication_factor = 2;
        bool enable_compression = true;
        std::string consistency_level = "eventual"; // "strong" or "eventual"
    };
    
    DistributedCache(const CacheConfig& config, ClusterManager& cluster);
    
    /**
     * @brief Store data in distributed cache
     * @param key Cache key
     * @param data Data to store
     * @param ttl_seconds Time to live (optional override)
     * @return Success status
     */
    bool put(const std::string& key, const std::vector<uint8_t>& data, 
             uint32_t ttl_seconds = 0);
    
    /**
     * @brief Retrieve data from distributed cache
     * @param key Cache key
     * @return Cached data (empty if not found)
     */
    std::vector<uint8_t> get(const std::string& key);
    
    /**
     * @brief Remove data from cache
     * @param key Cache key
     * @return Success status
     */
    bool remove(const std::string& key);
    
    /**
     * @brief Get cache statistics
     */
    struct CacheStats {
        uint64_t total_keys;
        size_t memory_usage_mb;
        double hit_rate;
        uint64_t get_operations;
        uint64_t put_operations;
        double average_latency_ms;
    };
    
    CacheStats get_stats() const;

private:
    CacheConfig config_;
    ClusterManager& cluster_;
    
    std::unordered_map<std::string, std::vector<uint8_t>> local_cache_;
    std::unordered_map<std::string, uint64_t> expiry_times_;
    
    mutable std::shared_mutex cache_mutex_;
    
    std::string hash_key_to_node(const std::string& key);
    std::vector<uint8_t> compress_data(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompress_data(const std::vector<uint8_t>& compressed_data);
};

/**
 * @brief Distributed stream processing engine
 */
class StreamProcessor {
public:
    using StreamFunction = std::function<void(const std::string& symbol, 
                                            const std::vector<uint8_t>& data)>;
    
    struct StreamConfig {
        size_t buffer_size = 10000;
        size_t batch_size = 100;
        uint32_t flush_interval_ms = 10;
        bool enable_exactly_once = true;
        size_t checkpoint_interval_ms = 5000;
    };
    
    StreamProcessor(const StreamConfig& config, ClusterManager& cluster);
    
    /**
     * @brief Create distributed stream processing pipeline
     * @param stream_name Unique stream identifier
     * @param input_sources List of input data sources
     * @param processing_function Stream processing function
     * @return Stream handle
     */
    std::string create_stream(const std::string& stream_name,
                             const std::vector<std::string>& input_sources,
                             StreamFunction processing_function);
    
    /**
     * @brief Start stream processing
     * @param stream_handle Stream to start
     */
    void start_stream(const std::string& stream_handle);
    
    /**
     * @brief Stop stream processing
     * @param stream_handle Stream to stop
     */
    void stop_stream(const std::string& stream_handle);
    
    /**
     * @brief Get stream processing statistics
     */
    struct StreamStats {
        uint64_t messages_processed;
        double throughput_msgs_per_sec;
        double average_latency_ms;
        uint64_t checkpoint_count;
        size_t backlog_size;
    };
    
    StreamStats get_stream_stats(const std::string& stream_handle) const;

private:
    StreamConfig config_;
    ClusterManager& cluster_;
    
    struct StreamInfo {
        std::string name;
        std::vector<std::string> sources;
        StreamFunction processor;
        std::atomic<bool> running;
        std::thread processing_thread;
        StreamStats stats;
    };
    
    std::unordered_map<std::string, std::unique_ptr<StreamInfo>> streams_;
    
    void process_stream(StreamInfo& stream_info);
    void checkpoint_stream(const std::string& stream_handle);
};

/**
 * @brief Distributed machine learning training
 */
class DistributedML {
public:
    struct TrainingConfig {
        std::string algorithm = "gradient_descent";
        size_t batch_size = 1000;
        double learning_rate = 0.01;
        size_t max_iterations = 10000;
        double convergence_threshold = 1e-6;
        bool use_parameter_server = true;
    };
    
    DistributedML(const TrainingConfig& config, ClusterManager& cluster);
    
    /**
     * @brief Train model across distributed cluster
     * @param training_data Distributed training dataset
     * @param model_architecture Neural network architecture
     * @return Trained model parameters
     */
    std::vector<double> train_distributed_model(
        const std::vector<std::vector<double>>& training_data,
        const std::vector<size_t>& model_architecture);
    
    /**
     * @brief Perform distributed hyperparameter optimization
     * @param parameter_ranges Hyperparameter search space
     * @param objective_function Optimization objective
     * @return Optimal hyperparameters
     */
    std::unordered_map<std::string, double> optimize_hyperparameters(
        const std::unordered_map<std::string, std::pair<double, double>>& parameter_ranges,
        std::function<double(const std::unordered_map<std::string, double>&)> objective_function);

private:
    TrainingConfig config_;
    ClusterManager& cluster_;
    
    void parameter_server_loop();
    std::vector<double> aggregate_gradients(const std::vector<std::vector<double>>& gradients);
};

} // namespace distributed
} // namespace feedhandler