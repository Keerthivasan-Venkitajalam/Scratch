#pragma once

#include <vector>
#include <memory>
#include <array>
#include <functional>
#include "common/tick.hpp"

namespace feedhandler {
namespace ml {

/**
 * @brief Real-time neural network for price prediction and anomaly detection
 * 
 * This system uses lightweight neural networks optimized for ultra-low latency
 * inference to predict price movements and detect market anomalies in real-time.
 * 
 * Features:
 * - Sub-microsecond inference time
 * - Online learning with streaming data
 * - Quantized neural networks for speed
 * - Hardware-accelerated matrix operations
 * - Ensemble prediction models
 */
class NeuralPredictor {
public:
    struct Config {
        size_t input_features = 50;        // Technical indicators
        size_t hidden_layers = 3;
        size_t neurons_per_layer = 64;
        double learning_rate = 0.001;
        size_t lookback_window = 100;      // Historical ticks to consider
        bool use_quantization = true;      // 8-bit quantized weights
        bool enable_online_learning = true;
    };
    
    struct Prediction {
        double price_direction;     // -1 to 1 (down to up)
        double confidence;          // 0 to 1
        double volatility_forecast; // Expected volatility
        bool anomaly_detected;      // Market anomaly flag
        uint64_t prediction_time_ns; // Inference latency
    };
    
    NeuralPredictor(const Config& config = {});
    
    /**
     * @brief Make real-time prediction from tick data
     * @param recent_ticks Historical tick data
     * @param current_tick Current tick to predict from
     * @return Prediction result
     */
    Prediction predict(const std::vector<common::Tick>& recent_ticks,
                      const common::Tick& current_tick);
    
    /**
     * @brief Update model with new market data (online learning)
     * @param ticks Training data
     * @param actual_outcomes Actual price movements
     */
    void update_model(const std::vector<common::Tick>& ticks,
                     const std::vector<double>& actual_outcomes);
    
    /**
     * @brief Extract technical indicators from tick data
     * @param ticks Historical tick data
     * @return Feature vector for neural network
     */
    std::vector<double> extract_features(const std::vector<common::Tick>& ticks);
    
    /**
     * @brief Get model performance metrics
     */
    struct ModelMetrics {
        double accuracy;
        double precision;
        double recall;
        double f1_score;
        double inference_time_ns;
        size_t predictions_made;
        double sharpe_ratio;
    };
    
    ModelMetrics get_metrics() const;
    
    /**
     * @brief Save model to file
     */
    void save_model(const std::string& filepath);
    
    /**
     * @brief Load model from file
     */
    void load_model(const std::string& filepath);

private:
    Config config_;
    
    // Quantized neural network weights (8-bit for speed)
    std::vector<std::vector<int8_t>> quantized_weights_;
    std::vector<std::vector<double>> bias_vectors_;
    
    // Feature extraction components
    std::array<double, 20> sma_values_;     // Simple moving averages
    std::array<double, 10> ema_values_;     // Exponential moving averages
    std::array<double, 5> rsi_values_;      // RSI indicators
    std::array<double, 3> bollinger_bands_; // Bollinger band values
    
    // Performance tracking
    mutable ModelMetrics metrics_;
    std::vector<double> prediction_history_;
    std::vector<double> actual_history_;
    
    // SIMD-optimized matrix operations
    void simd_matrix_multiply(const std::vector<double>& input,
                             const std::vector<std::vector<int8_t>>& weights,
                             std::vector<double>& output);
    
    double relu_activation(double x) { return std::max(0.0, x); }
    double sigmoid_activation(double x) { return 1.0 / (1.0 + std::exp(-x)); }
    
    // Technical indicator calculations
    double calculate_sma(const std::vector<common::Tick>& ticks, size_t period);
    double calculate_ema(const std::vector<common::Tick>& ticks, size_t period);
    double calculate_rsi(const std::vector<common::Tick>& ticks, size_t period);
    std::array<double, 3> calculate_bollinger_bands(const std::vector<common::Tick>& ticks);
    
    // Anomaly detection using statistical methods
    bool detect_price_anomaly(const common::Tick& tick, 
                             const std::vector<common::Tick>& history);
};

/**
 * @brief Ensemble predictor combining multiple models
 */
class EnsemblePredictor {
public:
    EnsemblePredictor();
    
    void add_model(std::unique_ptr<NeuralPredictor> model);
    
    NeuralPredictor::Prediction predict_ensemble(
        const std::vector<common::Tick>& recent_ticks,
        const common::Tick& current_tick);

private:
    std::vector<std::unique_ptr<NeuralPredictor>> models_;
    std::vector<double> model_weights_;
};

} // namespace ml
} // namespace feedhandler