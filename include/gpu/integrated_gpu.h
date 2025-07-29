#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>

/**
 * Integrated GPU types
 */
enum class IntegratedGPUType {
    UNKNOWN,
    INTEL_HD,
    INTEL_IRIS,
    INTEL_ARC,
    AMD_VEGA,
    AMD_RDNA,
    APPLE_M1,
    APPLE_M2,
    APPLE_M3
};

/**
 * Integrated GPU capabilities
 */
struct IntegratedGPUInfo {
    IntegratedGPUType type;
    std::string name;
    std::string vendor;
    std::string version;
    size_t total_memory;        // Total GPU memory in bytes
    size_t available_memory;    // Available GPU memory in bytes
    size_t shared_memory;       // Shared system memory in bytes
    int compute_units;          // Number of compute units/execution units
    int max_work_group_size;    // Maximum work group size
    int max_clock_frequency;    // Maximum clock frequency in MHz
    bool supports_opencl;       // OpenCL support
    bool supports_vulkan;       // Vulkan compute support
    bool is_power_constrained;  // Laptop/mobile device
    float thermal_design_power; // TDP in watts
};

/**
 * Integrated GPU performance profile
 */
struct IntegratedGPUProfile {
    std::string name;
    int recommended_work_group_size;
    int recommended_batch_size;
    float memory_usage_ratio;      // Percentage of GPU memory to use
    int thread_count_multiplier;   // Multiplier for compute units
    bool enable_memory_pooling;
    bool enable_thermal_throttling;
    std::map<std::string, std::string> compiler_options;
};

/**
 * Integrated GPU detector and manager
 */
class IntegratedGPUManager {
public:
    IntegratedGPUManager();
    ~IntegratedGPUManager() = default;

    /**
     * Detect available integrated GPUs
     * @return vector of detected integrated GPUs
     */
    std::vector<IntegratedGPUInfo> detect_integrated_gpus();

    /**
     * Get the best integrated GPU for recovery operations
     * @return pointer to best GPU info, nullptr if none found
     */
    std::unique_ptr<IntegratedGPUInfo> get_best_integrated_gpu();

    /**
     * Get performance profile for specific GPU type
     * @param type Integrated GPU type
     * @return performance profile
     */
    IntegratedGPUProfile get_performance_profile(IntegratedGPUType type);

    /**
     * Auto-configure settings for integrated GPU
     * @param gpu_info GPU information
     * @return recommended configuration map
     */
    std::map<std::string, std::string> auto_configure(const IntegratedGPUInfo& gpu_info);

    /**
     * Check if system is power constrained (laptop/mobile)
     * @return true if power constrained
     */
    bool is_power_constrained_system();

    /**
     * Get thermal throttling threshold
     * @param gpu_info GPU information
     * @return temperature threshold in Celsius
     */
    float get_thermal_threshold(const IntegratedGPUInfo& gpu_info);

    /**
     * Estimate performance relative to dedicated GPU
     * @param gpu_info GPU information
     * @return performance ratio (0.0 to 1.0)
     */
    float estimate_performance_ratio(const IntegratedGPUInfo& gpu_info);

private:
    std::vector<IntegratedGPUProfile> profiles_;

    // Detection methods
    std::vector<IntegratedGPUInfo> detect_intel_gpus();
    std::vector<IntegratedGPUInfo> detect_amd_gpus();
    std::vector<IntegratedGPUInfo> detect_apple_gpus();
    std::vector<IntegratedGPUInfo> detect_nvidia_integrated_gpus();

    // GPU type identification
    IntegratedGPUType identify_intel_gpu(const std::string& device_name);
    IntegratedGPUType identify_amd_gpu(const std::string& device_name);
    IntegratedGPUType identify_apple_gpu(const std::string& device_name);

    // System information
    bool detect_laptop_system();
    size_t get_system_memory();
    int get_cpu_core_count();

    // Profile initialization
    void initialize_profiles();
    IntegratedGPUProfile create_intel_hd_profile();
    IntegratedGPUProfile create_intel_iris_profile();
    IntegratedGPUProfile create_intel_arc_profile();
    IntegratedGPUProfile create_amd_vega_profile();
    IntegratedGPUProfile create_amd_rdna_profile();
    IntegratedGPUProfile create_apple_m1_profile();
    IntegratedGPUProfile create_apple_m2_profile();
    IntegratedGPUProfile create_apple_m3_profile();
};

/**
 * Integrated GPU OpenCL optimizer
 */
class IntegratedGPUOptimizer {
public:
    explicit IntegratedGPUOptimizer(const IntegratedGPUInfo& gpu_info);
    ~IntegratedGPUOptimizer() = default;

    /**
     * Optimize work group size for the GPU
     * @param base_work_group_size Base work group size
     * @return optimized work group size
     */
    int optimize_work_group_size(int base_work_group_size);

    /**
     * Optimize batch size for memory constraints
     * @param base_batch_size Base batch size
     * @return optimized batch size
     */
    int optimize_batch_size(int base_batch_size);

    /**
     * Get optimal memory buffer size
     * @return buffer size in bytes
     */
    size_t get_optimal_buffer_size();

    /**
     * Generate OpenCL compiler options
     * @return compiler options string
     */
    std::string get_compiler_options();

    /**
     * Check if thermal throttling is needed
     * @return true if throttling recommended
     */
    bool should_enable_thermal_throttling();

    /**
     * Get recommended CPU/GPU work ratio
     * @return ratio (0.0 = all CPU, 1.0 = all GPU)
     */
    float get_cpu_gpu_work_ratio();

private:
    IntegratedGPUInfo gpu_info_;
    
    // Optimization helpers
    int calculate_optimal_work_groups();
    size_t calculate_memory_overhead();
    bool is_memory_bandwidth_limited();
};
