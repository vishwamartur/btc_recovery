#pragma once

#ifdef ENABLE_CUDA

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <string>
#include <vector>
#include <memory>
#include <map>

/**
 * NVIDIA integrated GPU types
 */
enum class NVIDIAIntegratedType {
    UNKNOWN,
    TEGRA_X1,           // NVIDIA Tegra X1 (Shield TV, Jetson Nano)
    TEGRA_X2,           // NVIDIA Tegra X2 (Jetson TX2)
    TEGRA_XAVIER,       // NVIDIA Tegra Xavier (Jetson Xavier)
    TEGRA_ORIN,         // NVIDIA Tegra Orin (Jetson Orin)
    LAPTOP_MX_SERIES,   // NVIDIA MX series in laptops (shared memory)
    LAPTOP_GTX_MOBILE,  // Mobile GTX with shared memory
    GTX_1650_TI,        // NVIDIA GTX 1650 Ti (Turing mobile/desktop)
    GTX_1650_SERIES,    // NVIDIA GTX 1650 series (Turing)
    ARM_INTEGRATED      // ARM-based NVIDIA integrated graphics
};

/**
 * CUDA integrated GPU information
 */
struct CUDAIntegratedInfo {
    int device_id;
    NVIDIAIntegratedType type;
    std::string name;
    std::string compute_capability;
    size_t total_memory;
    size_t available_memory;
    size_t shared_memory_per_block;
    int multiprocessor_count;
    int max_threads_per_block;
    int max_threads_per_multiprocessor;
    int warp_size;
    int max_grid_size[3];
    int max_block_size[3];
    bool unified_memory_support;
    bool is_integrated;
    bool is_power_constrained;
    float memory_bandwidth_gb_s;
    int memory_bus_width;
    int memory_clock_rate;
    int gpu_clock_rate;
    float thermal_design_power;
};

/**
 * CUDA integrated GPU performance profile
 */
struct CUDAIntegratedProfile {
    std::string name;
    int recommended_threads_per_block;
    int recommended_blocks_per_grid;
    int recommended_shared_memory_size;
    int recommended_batch_size;
    float memory_usage_ratio;
    bool enable_unified_memory;
    bool enable_memory_pooling;
    bool enable_thermal_throttling;
    bool use_streams;
    int stream_count;
    std::map<std::string, int> kernel_parameters;
};

/**
 * CUDA integrated GPU manager
 */
class CUDAIntegratedManager {
public:
    CUDAIntegratedManager();
    ~CUDAIntegratedManager();

    /**
     * Initialize CUDA runtime for integrated GPUs
     * @return true if successful
     */
    bool initialize();

    /**
     * Detect NVIDIA integrated GPUs
     * @return vector of detected integrated GPUs
     */
    std::vector<CUDAIntegratedInfo> detect_cuda_integrated_gpus();

    /**
     * Get the best CUDA integrated GPU
     * @return pointer to best GPU info, nullptr if none found
     */
    std::unique_ptr<CUDAIntegratedInfo> get_best_cuda_integrated_gpu();

    /**
     * Check if device is integrated GPU
     * @param device_id CUDA device ID
     * @return true if integrated
     */
    bool is_integrated_gpu(int device_id);

    /**
     * Get performance profile for integrated GPU
     * @param gpu_info GPU information
     * @return performance profile
     */
    CUDAIntegratedProfile get_performance_profile(const CUDAIntegratedInfo& gpu_info);

    /**
     * Auto-configure CUDA settings for integrated GPU
     * @param gpu_info GPU information
     * @return configuration map
     */
    std::map<std::string, std::string> auto_configure_cuda(const CUDAIntegratedInfo& gpu_info);

    /**
     * Estimate performance relative to discrete GPU
     * @param gpu_info GPU information
     * @return performance ratio (0.0 to 1.0)
     */
    float estimate_cuda_performance_ratio(const CUDAIntegratedInfo& gpu_info);

    /**
     * Get optimal memory allocation strategy
     * @param gpu_info GPU information
     * @param required_memory Required memory in bytes
     * @return allocation strategy
     */
    std::string get_memory_allocation_strategy(const CUDAIntegratedInfo& gpu_info, size_t required_memory);

    /**
     * Check thermal throttling status
     * @param device_id CUDA device ID
     * @return true if throttling detected
     */
    bool is_thermal_throttling(int device_id);

    /**
     * Get current GPU temperature (if available)
     * @param device_id CUDA device ID
     * @return temperature in Celsius, -1 if not available
     */
    float get_gpu_temperature(int device_id);

    /**
     * Cleanup CUDA resources
     */
    void cleanup();

private:
    bool initialized_;
    std::vector<CUDAIntegratedProfile> profiles_;

    // Detection methods
    NVIDIAIntegratedType identify_nvidia_integrated_type(const cudaDeviceProp& props);
    bool is_tegra_device(const cudaDeviceProp& props);
    bool is_mobile_gpu(const cudaDeviceProp& props);
    bool has_unified_memory_architecture(const cudaDeviceProp& props);

    // Profile initialization
    void initialize_profiles();
    CUDAIntegratedProfile create_tegra_x1_profile();
    CUDAIntegratedProfile create_tegra_x2_profile();
    CUDAIntegratedProfile create_tegra_xavier_profile();
    CUDAIntegratedProfile create_tegra_orin_profile();
    CUDAIntegratedProfile create_gtx_1650_ti_profile();
    CUDAIntegratedProfile create_gtx_1650_series_profile();
    CUDAIntegratedProfile create_mx_series_profile();
    CUDAIntegratedProfile create_mobile_gtx_profile();
    CUDAIntegratedProfile create_arm_integrated_profile();

    // Utility methods
    float calculate_memory_bandwidth(const cudaDeviceProp& props);
    float estimate_tdp(const cudaDeviceProp& props, NVIDIAIntegratedType type);
    bool detect_power_constraints(int device_id);
};

/**
 * CUDA integrated GPU optimizer
 */
class CUDAIntegratedOptimizer {
public:
    explicit CUDAIntegratedOptimizer(const CUDAIntegratedInfo& gpu_info);
    ~CUDAIntegratedOptimizer() = default;

    /**
     * Optimize kernel launch parameters
     * @param base_threads_per_block Base threads per block
     * @param base_blocks_per_grid Base blocks per grid
     * @return optimized launch parameters
     */
    std::pair<int, int> optimize_launch_parameters(int base_threads_per_block, int base_blocks_per_grid);

    /**
     * Optimize memory allocation size
     * @param base_size Base allocation size
     * @return optimized size
     */
    size_t optimize_memory_allocation(size_t base_size);

    /**
     * Get optimal shared memory size
     * @return shared memory size in bytes
     */
    size_t get_optimal_shared_memory_size();

    /**
     * Check if unified memory should be used
     * @return true if unified memory is recommended
     */
    bool should_use_unified_memory();

    /**
     * Get recommended stream count
     * @return number of CUDA streams
     */
    int get_recommended_stream_count();

    /**
     * Calculate optimal batch size for memory constraints
     * @param base_batch_size Base batch size
     * @return optimized batch size
     */
    int calculate_optimal_batch_size(int base_batch_size);

    /**
     * Get memory coalescing recommendations
     * @return memory access pattern recommendations
     */
    std::vector<std::string> get_memory_coalescing_tips();

private:
    CUDAIntegratedInfo gpu_info_;

    // Optimization helpers
    int calculate_occupancy(int threads_per_block, size_t shared_memory_per_block);
    bool is_memory_bandwidth_limited();
    float get_memory_throughput_ratio();
    int get_optimal_thread_count();
};
