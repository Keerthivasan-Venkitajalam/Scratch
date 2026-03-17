#pragma once

#include <vector>
#include <complex>
#include <memory>
#include <functional>

namespace feedhandler {
namespace quantum {

/**
 * @brief Quantum-inspired optimization algorithms for portfolio management
 * 
 * This module implements quantum-inspired algorithms that leverage quantum
 * computing principles like superposition and entanglement for solving
 * complex optimization problems in trading and portfolio management.
 * 
 * Key algorithms:
 * - Quantum Approximate Optimization Algorithm (QAOA)
 * - Variational Quantum Eigensolver (VQE) for risk optimization
 * - Quantum-inspired genetic algorithms
 * - Adiabatic quantum computation simulation
 */
class QuantumOptimizer {
public:
    using Complex = std::complex<double>;
    using QuantumState = std::vector<Complex>;
    using CostFunction = std::function<double(const std::vector<double>&)>;
    
    struct OptimizationResult {
        std::vector<double> optimal_weights;
        double optimal_value;
        size_t iterations;
        double convergence_time_ms;
        double quantum_advantage_ratio; // Speedup vs classical
    };
    
    /**
     * @brief Portfolio optimization using quantum-inspired algorithms
     * @param expected_returns Expected returns for each asset
     * @param covariance_matrix Risk covariance matrix
     * @param risk_tolerance Risk tolerance parameter
     * @return Optimal portfolio weights
     */
    OptimizationResult optimize_portfolio(
        const std::vector<double>& expected_returns,
        const std::vector<std::vector<double>>& covariance_matrix,
        double risk_tolerance);
    
    /**
     * @brief Quantum Approximate Optimization Algorithm (QAOA)
     * @param cost_function Objective function to minimize
     * @param num_variables Number of optimization variables
     * @param num_layers QAOA circuit depth
     * @return Optimization result
     */
    OptimizationResult qaoa_optimize(const CostFunction& cost_function,
                                    size_t num_variables,
                                    size_t num_layers = 3);
    
    /**
     * @brief Variational Quantum Eigensolver for risk minimization
     * @param hamiltonian Risk Hamiltonian matrix
     * @param initial_state Initial quantum state
     * @return Ground state (minimum risk configuration)
     */
    OptimizationResult vqe_minimize_risk(
        const std::vector<std::vector<Complex>>& hamiltonian,
        const QuantumState& initial_state);
    
    /**
     * @brief Quantum-inspired genetic algorithm
     * @param fitness_function Fitness evaluation function
     * @param population_size Size of quantum population
     * @param generations Number of evolution generations
     * @return Best solution found
     */
    OptimizationResult quantum_genetic_algorithm(
        const CostFunction& fitness_function,
        size_t population_size,
        size_t generations);
    
    /**
     * @brief Simulate adiabatic quantum computation
     * @param initial_hamiltonian Starting Hamiltonian
     * @param final_hamiltonian Target Hamiltonian
     * @param evolution_time Total evolution time
     * @return Final quantum state
     */
    QuantumState adiabatic_evolution(
        const std::vector<std::vector<Complex>>& initial_hamiltonian,
        const std::vector<std::vector<Complex>>& final_hamiltonian,
        double evolution_time);

private:
    // Quantum gate operations
    void apply_hadamard(QuantumState& state, size_t qubit);
    void apply_cnot(QuantumState& state, size_t control, size_t target);
    void apply_rotation_z(QuantumState& state, size_t qubit, double angle);
    void apply_rotation_x(QuantumState& state, size_t qubit, double angle);
    
    // Quantum measurement
    std::vector<double> measure_expectation(const QuantumState& state,
                                          const std::vector<std::vector<Complex>>& observable);
    
    // Classical optimization helpers
    std::vector<double> nelder_mead_optimize(const CostFunction& func,
                                           const std::vector<double>& initial_guess);
    
    // Quantum circuit simulation
    QuantumState simulate_qaoa_circuit(const std::vector<double>& parameters,
                                      const CostFunction& cost_function,
                                      size_t num_qubits,
                                      size_t num_layers);
    
    // Performance tracking
    mutable size_t total_function_evaluations_;
    mutable double total_optimization_time_;
};

/**
 * @brief Quantum-inspired risk management system
 */
class QuantumRiskManager {
public:
    struct RiskMetrics {
        double value_at_risk_95;
        double expected_shortfall;
        double maximum_drawdown;
        double quantum_coherence_measure;
        std::vector<double> risk_decomposition;
    };
    
    /**
     * @brief Calculate quantum-enhanced risk metrics
     * @param portfolio_weights Current portfolio allocation
     * @param historical_returns Historical return data
     * @return Comprehensive risk analysis
     */
    RiskMetrics calculate_quantum_risk(
        const std::vector<double>& portfolio_weights,
        const std::vector<std::vector<double>>& historical_returns);
    
    /**
     * @brief Quantum entanglement-based correlation analysis
     * @param asset_returns Return series for multiple assets
     * @return Quantum correlation matrix
     */
    std::vector<std::vector<Complex>> quantum_correlation_analysis(
        const std::vector<std::vector<double>>& asset_returns);

private:
    QuantumOptimizer optimizer_;
};

/**
 * @brief Quantum machine learning for pattern recognition
 */
class QuantumML {
public:
    /**
     * @brief Quantum Support Vector Machine for classification
     * @param training_data Feature vectors
     * @param labels Classification labels
     * @return Trained quantum classifier
     */
    struct QuantumClassifier {
        QuantumState quantum_weights;
        std::vector<double> support_vectors;
        double bias;
    };
    
    QuantumClassifier train_quantum_svm(
        const std::vector<std::vector<double>>& training_data,
        const std::vector<int>& labels);
    
    /**
     * @brief Classify new data point using quantum classifier
     * @param classifier Trained quantum classifier
     * @param data_point Input feature vector
     * @return Classification result and confidence
     */
    std::pair<int, double> classify(const QuantumClassifier& classifier,
                                   const std::vector<double>& data_point);

private:
    QuantumOptimizer optimizer_;
};

} // namespace quantum
} // namespace feedhandler