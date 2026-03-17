#include "ml/neural_predictor.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <random>
#include <fstream>
#include <chrono>

#ifdef __ARM_NEON
#include <arm_neon.h>
#else
#include <immintrin.h>
#endif

namespace feedhandler {
namespace ml {

NeuralPredictor::NeuralPredictor(const Config& config) 
    : config_(config), metrics_{} {
    
    // Initialize quantized neural network weights
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> weight_dist(0.0, 0.1);
    
    // Initialize weight matrices for each layer
    quantized_weights_.resize(config_.hidden_layers + 1);
    bias_vectors_.resize(config_.hidden_layers + 1);
    
    size_t prev_size = config_.input_features;
    for (size_t layer = 0; layer < config_.hidden_layers; ++layer) {
        size_t current_size = config_.neurons_per_layer;
        
        quantized_weights_[layer].resize(prev_size * current_size);
        bias_vectors_[layer].resize(current_size);
        
        // Initialize with quantized weights (-127 to 127)
        for (size_t i = 0; i < quantized_weights_[layer].size(); ++i) {
            double weight = weight_dist(gen);
            quantized_weights_[layer][i] = static_cast<int8_t>(
                std::clamp(weight * 127.0, -127.0, 127.0));
        }
        
        for (size_t i = 0; i < bias_vectors_[layer].size(); ++i) {
            bias_vectors_[layer][i] = weight_dist(gen);
        }
        
        prev_size = current_size;
    }
    
    // Output layer (3 outputs: direction, confidence, volatility)
    quantized_weights_.back().resize(prev_size * 3);
    bias_vectors_.back().resize(3);
    
    for (size_t i = 0; i < quantized_weights_.back().size(); ++i) {
        double weight = weight_dist(gen);
        quantized_weights_.back()[i] = static_cast<int8_t>(
            std::clamp(weight * 127.0, -127.0, 127.0));
    }
    
    for (size_t i = 0; i < 3; ++i) {
        bias_vectors_.back()[i] = weight_dist(gen);
    }
    
    // Initialize technical indicator arrays
    sma_values_.fill(0.0);
    ema_values_.fill(0.0);
    rsi_values_.fill(50.0);
    bollinger_bands_.fill(0.0);
}

NeuralPredictor::Prediction NeuralPredictor::predict(
    const std::vector<common::Tick>& recent_ticks,
    const common::Tick& current_tick) {
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Extract features from tick data
    std::vector<double> features = extract_features(recent_ticks);
    
    // Add current tick features
    features.push_back(static_cast<double>(current_tick.price) / 10000.0);
    features.push_back(static_cast<double>(current_tick.qty));
    features.push_back(current_tick.side == 'B' ? 1.0 : -1.0);
    
    // Ensure feature vector is correct size
    features.resize(config_.input_features, 0.0);
    
    // Forward pass through neural network
    std::vector<double> current_layer = features;
    std::vector<double> next_layer;
    
    for (size_t layer = 0; layer < quantized_weights_.size(); ++layer) {
        size_t input_size = current_layer.size();
        size_t output_size = bias_vectors_[layer].size();
        
        next_layer.resize(output_size);
        
        // SIMD-optimized matrix multiplication with quantized weights
        simd_matrix_multiply(current_layer, {quantized_weights_[layer]}, next_layer);
        
        // Add bias and apply activation
        for (size_t i = 0; i < output_size; ++i) {
            next_layer[i] += bias_vectors_[layer][i];
            
            if (layer < quantized_weights_.size() - 1) {
                next_layer[i] = relu_activation(next_layer[i]);
            } else {
                // Output layer uses different activations
                if (i == 0) { // Price direction
                    next_layer[i] = std::tanh(next_layer[i]);
                } else { // Confidence and volatility
                    next_layer[i] = sigmoid_activation(next_layer[i]);
                }
            }
        }
        
        current_layer = std::move(next_layer);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto inference_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end_time - start_time).count();
    
    // Create prediction result
    Prediction prediction;
    prediction.price_direction = current_layer[0];
    prediction.confidence = current_layer[1];
    prediction.volatility_forecast = current_layer[2];
    prediction.anomaly_detected = detect_price_anomaly(current_tick, recent_ticks);
    prediction.prediction_time_ns = inference_time;
    
    // Update metrics
    metrics_.predictions_made++;
    metrics_.inference_time_ns = (metrics_.inference_time_ns * 0.9) + (inference_time * 0.1);
    
    return prediction;
}

std::vector<double> NeuralPredictor::extract_features(
    const std::vector<common::Tick>& ticks) {
    
    std::vector<double> features;
    features.reserve(config_.input_features);
    
    if (ticks.empty()) {
        features.resize(config_.input_features, 0.0);
        return features;
    }
    
    // Calculate technical indicators
    std::vector<size_t> sma_periods = {5, 10, 20, 50, 100, 200};
    for (size_t period : sma_periods) {
        double sma = calculate_sma(ticks, period);
        features.push_back(sma);
    }
    
    std::vector<size_t> ema_periods = {5, 10, 20, 50};
    for (size_t period : ema_periods) {
        double ema = calculate_ema(ticks, period);
        features.push_back(ema);
    }
    
    // RSI indicators
    std::vector<size_t> rsi_periods = {14, 21, 30};
    for (size_t period : rsi_periods) {
        double rsi = calculate_rsi(ticks, period);
        features.push_back(rsi / 100.0); // Normalize to 0-1
    }
    
    // Bollinger Bands
    auto bb = calculate_bollinger_bands(ticks);
    features.insert(features.end(), bb.begin(), bb.end());
    
    // Price momentum features
    if (ticks.size() >= 10) {
        double current_price = static_cast<double>(ticks.back().price) / 10000.0;
        double price_10_ago = static_cast<double>(ticks[ticks.size()-10].price) / 10000.0;
        features.push_back((current_price - price_10_ago) / price_10_ago);
    } else {
        features.push_back(0.0);
    }
    
    // Volume features
    if (ticks.size() >= 5) {
        double avg_volume = 0.0;
        for (size_t i = ticks.size() - 5; i < ticks.size(); ++i) {
            avg_volume += ticks[i].qty;
        }
        avg_volume /= 5.0;
        features.push_back(avg_volume / 1000.0); // Normalize
    } else {
        features.push_back(0.0);
    }
    
    // Volatility features
    if (ticks.size() >= 20) {
        std::vector<double> returns;
        for (size_t i = ticks.size() - 19; i < ticks.size(); ++i) {
            double prev_price = static_cast<double>(ticks[i-1].price) / 10000.0;
            double curr_price = static_cast<double>(ticks[i].price) / 10000.0;
            returns.push_back(std::log(curr_price / prev_price));
        }
        
        double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
        double variance = 0.0;
        for (double ret : returns) {
            variance += (ret - mean_return) * (ret - mean_return);
        }
        variance /= returns.size();
        features.push_back(std::sqrt(variance) * 100.0); // Volatility in %
    } else {
        features.push_back(0.0);
    }
    
    // Ensure we have exactly the right number of features
    features.resize(config_.input_features, 0.0);
    
    return features;
}

void NeuralPredictor::simd_matrix_multiply(const std::vector<double>& input,
                                          const std::vector<std::vector<int8_t>>& weights,
                                          std::vector<double>& output) {
    
    const auto& weight_matrix = weights[0];
    size_t input_size = input.size();
    size_t output_size = output.size();
    
    // SIMD-optimized matrix multiplication
#ifdef __ARM_NEON
    // ARM NEON implementation
    for (size_t out_idx = 0; out_idx < output_size; ++out_idx) {
        float32x4_t sum_vec = vdupq_n_f32(0.0f);
        
        size_t simd_end = (input_size / 4) * 4;
        for (size_t in_idx = 0; in_idx < simd_end; in_idx += 4) {
            // Load input values
            float32x4_t input_vec = vld1q_f32(reinterpret_cast<const float*>(&input[in_idx]));
            
            // Load and convert quantized weights
            int8x8_t weight_i8 = vld1_s8(&weight_matrix[out_idx * input_size + in_idx]);
            int16x4_t weight_i16 = vget_low_s16(vmovl_s8(weight_i8));
            float32x4_t weight_vec = vcvtq_f32_s32(vmovl_s16(weight_i16));
            
            // Scale quantized weights back to float range
            weight_vec = vmulq_n_f32(weight_vec, 1.0f / 127.0f);
            
            // Multiply and accumulate
            sum_vec = vmlaq_f32(sum_vec, input_vec, weight_vec);
        }
        
        // Sum the vector elements
        float sum = vaddvq_f32(sum_vec);
        
        // Handle remaining elements
        for (size_t in_idx = simd_end; in_idx < input_size; ++in_idx) {
            int8_t quantized_weight = weight_matrix[out_idx * input_size + in_idx];
            double weight = static_cast<double>(quantized_weight) / 127.0;
            sum += input[in_idx] * weight;
        }
        
        output[out_idx] = sum;
    }
#else
    // x86 AVX implementation
    for (size_t out_idx = 0; out_idx < output_size; ++out_idx) {
        __m256d sum_vec = _mm256_setzero_pd();
        
        size_t simd_end = (input_size / 4) * 4;
        for (size_t in_idx = 0; in_idx < simd_end; in_idx += 4) {
            // Load input values
            __m256d input_vec = _mm256_loadu_pd(&input[in_idx]);
            
            // Load and convert quantized weights
            __m128i weight_i8 = _mm_loadl_epi64(
                reinterpret_cast<const __m128i*>(&weight_matrix[out_idx * input_size + in_idx]));
            __m128i weight_i32 = _mm_cvtepi8_epi32(weight_i8);
            __m256d weight_vec = _mm256_cvtepi32_pd(weight_i32);
            
            // Scale quantized weights back to double range
            weight_vec = _mm256_mul_pd(weight_vec, _mm256_set1_pd(1.0 / 127.0));
            
            // Multiply and accumulate
            sum_vec = _mm256_fmadd_pd(input_vec, weight_vec, sum_vec);
        }
        
        // Sum the vector elements
        double sum_array[4];
        _mm256_storeu_pd(sum_array, sum_vec);
        double sum = sum_array[0] + sum_array[1] + sum_array[2] + sum_array[3];
        
        // Handle remaining elements
        for (size_t in_idx = simd_end; in_idx < input_size; ++in_idx) {
            int8_t quantized_weight = weight_matrix[out_idx * input_size + in_idx];
            double weight = static_cast<double>(quantized_weight) / 127.0;
            sum += input[in_idx] * weight;
        }
        
        output[out_idx] = sum;
    }
#endif
}

double NeuralPredictor::calculate_sma(const std::vector<common::Tick>& ticks, size_t period) {
    if (ticks.size() < period) return 0.0;
    
    double sum = 0.0;
    for (size_t i = ticks.size() - period; i < ticks.size(); ++i) {
        sum += static_cast<double>(ticks[i].price) / 10000.0;
    }
    return sum / period;
}

double NeuralPredictor::calculate_ema(const std::vector<common::Tick>& ticks, size_t period) {
    if (ticks.empty()) return 0.0;
    
    double alpha = 2.0 / (period + 1.0);
    double ema = static_cast<double>(ticks[0].price) / 10000.0;
    
    for (size_t i = 1; i < ticks.size(); ++i) {
        double price = static_cast<double>(ticks[i].price) / 10000.0;
        ema = alpha * price + (1.0 - alpha) * ema;
    }
    
    return ema;
}

double NeuralPredictor::calculate_rsi(const std::vector<common::Tick>& ticks, size_t period) {
    if (ticks.size() < period + 1) return 50.0;
    
    double gain_sum = 0.0;
    double loss_sum = 0.0;
    
    for (size_t i = ticks.size() - period; i < ticks.size(); ++i) {
        double prev_price = static_cast<double>(ticks[i-1].price) / 10000.0;
        double curr_price = static_cast<double>(ticks[i].price) / 10000.0;
        double change = curr_price - prev_price;
        
        if (change > 0) {
            gain_sum += change;
        } else {
            loss_sum += -change;
        }
    }
    
    if (loss_sum == 0.0) return 100.0;
    
    double rs = (gain_sum / period) / (loss_sum / period);
    return 100.0 - (100.0 / (1.0 + rs));
}

std::array<double, 3> NeuralPredictor::calculate_bollinger_bands(
    const std::vector<common::Tick>& ticks) {
    
    if (ticks.size() < 20) return {0.0, 0.0, 0.0};
    
    // Calculate 20-period SMA
    double sma = calculate_sma(ticks, 20);
    
    // Calculate standard deviation
    double variance = 0.0;
    for (size_t i = ticks.size() - 20; i < ticks.size(); ++i) {
        double price = static_cast<double>(ticks[i].price) / 10000.0;
        variance += (price - sma) * (price - sma);
    }
    double std_dev = std::sqrt(variance / 20.0);
    
    return {sma - 2.0 * std_dev, sma, sma + 2.0 * std_dev}; // Lower, Middle, Upper
}

bool NeuralPredictor::detect_price_anomaly(const common::Tick& tick,
                                          const std::vector<common::Tick>& history) {
    if (history.size() < 50) return false;
    
    double current_price = static_cast<double>(tick.price) / 10000.0;
    
    // Calculate recent price statistics
    double sum = 0.0;
    for (size_t i = history.size() - 50; i < history.size(); ++i) {
        sum += static_cast<double>(history[i].price) / 10000.0;
    }
    double mean = sum / 50.0;
    
    double variance = 0.0;
    for (size_t i = history.size() - 50; i < history.size(); ++i) {
        double price = static_cast<double>(history[i].price) / 10000.0;
        variance += (price - mean) * (price - mean);
    }
    double std_dev = std::sqrt(variance / 50.0);
    
    // Detect anomaly if price is more than 3 standard deviations from mean
    double z_score = std::abs(current_price - mean) / std_dev;
    return z_score > 3.0;
}

NeuralPredictor::ModelMetrics NeuralPredictor::get_metrics() const {
    return metrics_;
}

void NeuralPredictor::save_model(const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    
    // Save configuration
    file.write(reinterpret_cast<const char*>(&config_), sizeof(config_));
    
    // Save quantized weights
    for (const auto& layer_weights : quantized_weights_) {
        size_t size = layer_weights.size();
        file.write(reinterpret_cast<const char*>(&size), sizeof(size));
        file.write(reinterpret_cast<const char*>(layer_weights.data()), 
                  size * sizeof(int8_t));
    }
    
    // Save biases
    for (const auto& layer_biases : bias_vectors_) {
        size_t size = layer_biases.size();
        file.write(reinterpret_cast<const char*>(&size), sizeof(size));
        file.write(reinterpret_cast<const char*>(layer_biases.data()),
                  size * sizeof(double));
    }
}

void NeuralPredictor::load_model(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    
    // Load configuration
    file.read(reinterpret_cast<char*>(&config_), sizeof(config_));
    
    // Load quantized weights
    quantized_weights_.clear();
    for (size_t layer = 0; layer < config_.hidden_layers + 1; ++layer) {
        size_t size;
        file.read(reinterpret_cast<char*>(&size), sizeof(size));
        
        std::vector<int8_t> layer_weights(size);
        file.read(reinterpret_cast<char*>(layer_weights.data()),
                 size * sizeof(int8_t));
        quantized_weights_.push_back(std::move(layer_weights));
    }
    
    // Load biases
    bias_vectors_.clear();
    for (size_t layer = 0; layer < config_.hidden_layers + 1; ++layer) {
        size_t size;
        file.read(reinterpret_cast<char*>(&size), sizeof(size));
        
        std::vector<double> layer_biases(size);
        file.read(reinterpret_cast<char*>(layer_biases.data()),
                 size * sizeof(double));
        bias_vectors_.push_back(std::move(layer_biases));
    }
}

} // namespace ml
} // namespace feedhandler