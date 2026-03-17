#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include <chrono>

namespace feedhandler {
namespace network {

/**
 * @brief Ultra-low latency RDMA (Remote Direct Memory Access) transport
 * 
 * This transport layer bypasses the kernel and directly accesses network
 * hardware, achieving sub-100ns network latency for market data delivery.
 * 
 * Features:
 * - Kernel bypass networking
 * - Zero-copy data transfer
 * - Hardware timestamping
 * - Multicast market data feeds
 * - InfiniBand and RoCE support
 */
class RDMATransport {
public:
    struct Config {
        std::string device_name = "mlx5_0";  // InfiniBand device
        uint16_t port = 1;
        size_t queue_depth = 1024;
        size_t max_message_size = 9000;      // Jumbo frames
        bool use_hardware_timestamps = true;
        bool enable_multicast = true;
        std::string multicast_group = "239.1.1.1";
    };
    
    struct NetworkStats {
        uint64_t messages_received;
        uint64_t bytes_received;
        uint64_t packet_drops;
        double average_latency_ns;
        double jitter_ns;
        uint64_t hardware_timestamp_errors;
    };
    
    using MessageCallback = std::function<void(const char*, size_t, uint64_t timestamp)>;
    
    RDMATransport(const Config& config = {});
    ~RDMATransport();
    
    /**
     * @brief Initialize RDMA connection
     * @param remote_address Remote server address
     * @param remote_port Remote server port
     * @return Success status
     */
    bool connect(const std::string& remote_address, uint16_t remote_port);
    
    /**
     * @brief Start receiving market data
     * @param callback Message processing callback
     */
    void start_receiving(MessageCallback callback);
    
    /**
     * @brief Stop receiving data
     */
    void stop_receiving();
    
    /**
     * @brief Send message with zero-copy
     * @param data Message data
     * @param length Message length
     * @return Success status
     */
    bool send_message(const void* data, size_t length);
    
    /**
     * @brief Get network performance statistics
     */
    NetworkStats get_stats() const;
    
    /**
     * @brief Enable hardware timestamping
     * @param enable Enable/disable hardware timestamps
     */
    void set_hardware_timestamping(bool enable);
    
    /**
     * @brief Join multicast group for market data
     * @param group_address Multicast group IP
     * @return Success status
     */
    bool join_multicast_group(const std::string& group_address);

private:
    Config config_;
    void* rdma_context_;
    void* completion_queue_;
    void* queue_pair_;
    void* memory_region_;
    
    std::atomic<bool> receiving_;
    std::thread receive_thread_;
    
    MessageCallback message_callback_;
    mutable NetworkStats stats_;
    
    bool initialize_rdma();
    void cleanup_rdma();
    void receive_loop();
    void process_completion();
    uint64_t get_hardware_timestamp();
};

/**
 * @brief Kernel bypass UDP transport using DPDK
 */
class DPDKTransport {
public:
    struct Config {
        std::string pci_device = "0000:01:00.0";
        uint16_t port_id = 0;
        size_t rx_queue_size = 2048;
        size_t tx_queue_size = 2048;
        size_t mbuf_pool_size = 8192;
        bool enable_rss = true;  // Receive Side Scaling
        uint16_t mtu = 9000;     // Jumbo frames
    };
    
    using PacketCallback = std::function<void(const char*, size_t, uint64_t timestamp)>;
    
    DPDKTransport(const Config& config = {});
    ~DPDKTransport();
    
    /**
     * @brief Initialize DPDK environment
     * @return Success status
     */
    bool initialize();
    
    /**
     * @brief Start packet processing
     * @param callback Packet processing callback
     */
    void start_processing(PacketCallback callback);
    
    /**
     * @brief Send packet with zero-copy
     * @param data Packet data
     * @param length Packet length
     * @param dest_ip Destination IP
     * @param dest_port Destination port
     * @return Success status
     */
    bool send_packet(const void* data, size_t length,
                    uint32_t dest_ip, uint16_t dest_port);
    
    /**
     * @brief Get packet processing statistics
     */
    struct PacketStats {
        uint64_t rx_packets;
        uint64_t tx_packets;
        uint64_t rx_dropped;
        uint64_t tx_dropped;
        double pps_rate;  // Packets per second
        double bandwidth_gbps;
    };
    
    PacketStats get_stats() const;

private:
    Config config_;
    void* dpdk_port_;
    void* mbuf_pool_;
    
    std::atomic<bool> processing_;
    std::vector<std::thread> worker_threads_;
    
    PacketCallback packet_callback_;
    mutable PacketStats stats_;
    
    bool setup_dpdk_port();
    void worker_loop(int queue_id);
    void process_rx_burst(int queue_id);
};

/**
 * @brief High-precision network timestamping
 */
class NetworkTimestamping {
public:
    /**
     * @brief Calibrate network timestamp offset
     * @param reference_server PTP reference server
     * @return Calibration offset in nanoseconds
     */
    static int64_t calibrate_timestamps(const std::string& reference_server);
    
    /**
     * @brief Get hardware timestamp from network packet
     * @param packet_data Raw packet data
     * @return Hardware timestamp in nanoseconds
     */
    static uint64_t extract_hardware_timestamp(const void* packet_data);
    
    /**
     * @brief Convert network timestamp to system time
     * @param network_timestamp Hardware timestamp
     * @return System time in nanoseconds
     */
    static uint64_t network_to_system_time(uint64_t network_timestamp);
    
    /**
     * @brief Measure one-way network latency
     * @param remote_address Remote server address
     * @return Latency in nanoseconds
     */
    static double measure_network_latency(const std::string& remote_address);

private:
    static int64_t timestamp_offset_;
    static std::chrono::steady_clock::time_point calibration_time_;
};

/**
 * @brief Market data multicast receiver with RDMA
 */
class MulticastReceiver {
public:
    struct MulticastConfig {
        std::vector<std::string> multicast_groups;
        std::string interface_ip;
        uint16_t base_port = 10000;
        size_t buffer_size = 64 * 1024;  // 64KB receive buffer
        bool use_rdma = true;
    };
    
    using FeedCallback = std::function<void(const std::string& symbol,
                                          const char* data, size_t length,
                                          uint64_t timestamp)>;
    
    MulticastReceiver(const MulticastConfig& config);
    
    /**
     * @brief Start receiving multicast feeds
     * @param callback Feed processing callback
     */
    void start_feeds(FeedCallback callback);
    
    /**
     * @brief Subscribe to specific symbol feeds
     * @param symbols List of symbols to subscribe
     */
    void subscribe_symbols(const std::vector<std::string>& symbols);
    
    /**
     * @brief Get feed statistics per symbol
     */
    std::map<std::string, NetworkStats> get_feed_stats() const;

private:
    MulticastConfig config_;
    std::vector<std::unique_ptr<RDMATransport>> transports_;
    FeedCallback feed_callback_;
    
    void setup_multicast_feeds();
    void process_multicast_message(const char* data, size_t length, uint64_t timestamp);
};

} // namespace network
} // namespace feedhandler