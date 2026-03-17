#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <thread>
#include "common/tick.hpp"

namespace feedhandler {
namespace analytics {

/**
 * @brief Real-time analytics engine for market microstructure analysis
 * 
 * This engine processes market data in real-time to compute advanced
 * analytics, detect patterns, and generate trading signals with
 * sub-microsecond latency.
 * 
 * Features:
 * - Real-time VWAP, TWAP calculations
 * - Order flow imbalance detection
 * - Market impact estimation
 * - Liquidity analysis
 * - Cross-asset correlation monitoring
 * - Regime change detection
 */
class RealtimeEngine {
public:
    struct Config {
        size_t max_symbols = 10000;
        size_t history_depth = 100000;     // Ticks to keep in memory
        double update_frequency_hz = 1000000; // 1MHz update rate
        bool enable_cross_asset_analysis = true;
        bool enable_regime_detection = true;
        size_t worker_threads = 8;
    };
    
    struct MarketMetrics {
        // Price metrics
        double vwap;                    // Volume-weighted average price
        double twap;                    // Time-weighted average price
        double microprice;              // Bid-ask midpoint
        double spread_bps;              // Bid-ask spread in basis points
        
        // Volume metrics
        uint64_t total_volume;
        double volume_rate;             // Volume per second
        double participation_rate;      // % of total market volume
        
        // Liquidity metrics
        double bid_depth;               // Total bid liquidity
        double ask_depth;               // Total ask liquidity
        double liquidity_imbalance;    // (bid_depth - ask_depth) / total
        double effective_spread;        // Realized transaction costs
        
        // Volatility metrics
        double realized_volatility;     // Recent price volatility
        double garman_klass_volatility; // High-low volatility estimator
        double parkinson_volatility;    // Range-based volatility
        
        // Market microstructure
        double order_flow_imbalance;    // Buy vs sell pressure
        double price_impact;            // Market impact per unit volume
        double resilience;              // Speed of spread recovery
        
        // Cross-asset metrics
        std::unordered_map<std::string, double> correlations;
        double beta_to_market;          // Systematic risk measure
        
        // Timing
        uint64_t last_update_ns;
        uint64_t calculation_time_ns;
    };
    
    using MetricsCallback = std::function<void(const std::string& symbol, 
                                             const MarketMetrics& metrics)>;
    using AlertCallback = std::function<void(const std::string& symbol,
                                           const std::string& alert_type,
                                           double severity)>;
    
    RealtimeEngine(const Config& config = {});
    ~RealtimeEngine();
    
    /**
     * @brief Process new market tick
     * @param symbol Instrument symbol
     * @param tick Market data tick
     */
    void process_tick(const std::string& symbol, const common::Tick& tick);
    
    /**
     * @brief Get current metrics for symbol
     * @param symbol Instrument symbol
     * @return Current market metrics
     */
    MarketMetrics get_metrics(const std::string& symbol) const;
    
    /**
     * @brief Register callback for metrics updates
     * @param callback Metrics update callback
     */
    void register_metrics_callback(MetricsCallback callback);
    
    /**
     * @brief Register callback for market alerts
     * @param callback Alert callback
     */
    void register_alert_callback(AlertCallback callback);
    
    /**
     * @brief Start real-time processing
     */
    void start();
    
    /**
     * @brief Stop real-time processing
     */
    void stop();
    
    /**
     * @brief Get engine performance statistics
     */
    struct EngineStats {
        uint64_t ticks_processed;
        uint64_t metrics_calculated;
        uint64_t alerts_generated;
        double processing_rate_hz;
        double average_latency_ns;
        double cpu_utilization;
        size_t memory_usage_mb;
    };
    
    EngineStats get_engine_stats() const;

private:
    Config config_;
    
    // Symbol data storage
    struct SymbolData {
        std::vector<common::Tick> tick_history;
        MarketMetrics current_metrics;
        std::atomic<uint64_t> last_update;
        
        // Rolling calculations
        double price_sum;
        double volume_sum;
        uint64_t tick_count;
        
        // Volatility calculation state
        std::vector<double> price_returns;
        double sum_squared_returns;
        
        // Order flow tracking
        double buy_volume;
        double sell_volume;
        std::vector<double> trade_sizes;
        
        // Liquidity tracking
        std::vector<std::pair<double, double>> bid_ask_history;
    };
    
    std::unordered_map<std::string, std::unique_ptr<SymbolData>> symbol_data_;
    mutable std::shared_mutex data_mutex_;
    
    // Processing threads
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> running_;
    
    // Callbacks
    MetricsCallback metrics_callback_;
    AlertCallback alert_callback_;
    
    // Performance tracking
    mutable EngineStats engine_stats_;
    std::atomic<uint64_t> processed_ticks_;
    
    // Processing methods
    void worker_loop(int worker_id);
    void calculate_metrics(const std::string& symbol, SymbolData& data);
    void update_vwap(SymbolData& data, const common::Tick& tick);
    void update_volatility(SymbolData& data, const common::Tick& tick);
    void update_liquidity_metrics(SymbolData& data, const common::Tick& tick);
    void update_order_flow(SymbolData& data, const common::Tick& tick);
    void detect_anomalies(const std::string& symbol, const SymbolData& data);
    
    // Cross-asset analysis
    void update_correlations();
    double calculate_correlation(const std::string& symbol1, const std::string& symbol2);
    
    // Alert generation
    void check_liquidity_alerts(const std::string& symbol, const MarketMetrics& metrics);
    void check_volatility_alerts(const std::string& symbol, const MarketMetrics& metrics);
    void check_flow_alerts(const std::string& symbol, const MarketMetrics& metrics);
};

/**
 * @brief Advanced pattern recognition engine
 */
class PatternEngine {
public:
    enum class PatternType {
        MOMENTUM_BREAKOUT,
        MEAN_REVERSION,
        VOLUME_SPIKE,
        LIQUIDITY_DROUGHT,
        CROSS_ASSET_ARBITRAGE,
        REGIME_CHANGE
    };
    
    struct Pattern {
        PatternType type;
        std::string symbol;
        double confidence;
        uint64_t detection_time;
        std::vector<double> parameters;
        std::string description;
    };
    
    using PatternCallback = std::function<void(const Pattern& pattern)>;
    
    PatternEngine();
    
    /**
     * @brief Analyze tick for patterns
     * @param symbol Instrument symbol
     * @param tick Market data tick
     * @param metrics Current market metrics
     * @return Detected patterns
     */
    std::vector<Pattern> analyze_patterns(const std::string& symbol,
                                        const common::Tick& tick,
                                        const RealtimeEngine::MarketMetrics& metrics);
    
    /**
     * @brief Register pattern detection callback
     * @param callback Pattern callback
     */
    void register_pattern_callback(PatternCallback callback);

private:
    PatternCallback pattern_callback_;
    
    // Pattern detection methods
    std::vector<Pattern> detect_momentum_patterns(const std::string& symbol,
                                                 const RealtimeEngine::MarketMetrics& metrics);
    std::vector<Pattern> detect_volume_patterns(const std::string& symbol,
                                               const common::Tick& tick);
    std::vector<Pattern> detect_liquidity_patterns(const std::string& symbol,
                                                   const RealtimeEngine::MarketMetrics& metrics);
};

/**
 * @brief Market regime detection using machine learning
 */
class RegimeDetector {
public:
    enum class MarketRegime {
        TRENDING_UP,
        TRENDING_DOWN,
        SIDEWAYS,
        HIGH_VOLATILITY,
        LOW_VOLATILITY,
        CRISIS,
        RECOVERY
    };
    
    struct RegimeState {
        MarketRegime current_regime;
        double confidence;
        uint64_t regime_start_time;
        std::vector<double> regime_probabilities;
    };
    
    RegimeDetector();
    
    /**
     * @brief Update regime detection with new market data
     * @param metrics Market metrics from multiple symbols
     */
    void update_regime(const std::unordered_map<std::string, RealtimeEngine::MarketMetrics>& metrics);
    
    /**
     * @brief Get current market regime
     * @return Current regime state
     */
    RegimeState get_current_regime() const;

private:
    RegimeState current_state_;
    std::vector<std::vector<double>> feature_history_;
    
    std::vector<double> extract_regime_features(
        const std::unordered_map<std::string, RealtimeEngine::MarketMetrics>& metrics);
    MarketRegime classify_regime(const std::vector<double>& features);
};

} // namespace analytics
} // namespace feedhandler