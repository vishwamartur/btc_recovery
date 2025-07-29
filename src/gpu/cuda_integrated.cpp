#ifdef ENABLE_CUDA

#include "gpu/cuda_integrated.h"
#include "utils/logger.h"
#include <algorithm>
#include <sstream>
#include <cmath>

CUDAIntegratedManager::CUDAIntegratedManager() : initialized_(false) {
    initialize_profiles();
}

CUDAIntegratedManager::~CUDAIntegratedManager() {
    cleanup();
}

bool CUDAIntegratedManager::initialize() {
    if (initialized_) {
        return true;
    }

    cudaError_t error = cudaGetDeviceCount(nullptr);
    if (error != cudaSuccess) {
        Logger::error("CUDA initialization failed: " + std::string(cudaGetErrorString(error)));
        return false;
    }

    initialized_ = true;
    Logger::info("CUDA integrated GPU manager initialized");
    return true;
}

std::vector<CUDAIntegratedInfo> CUDAIntegratedManager::detect_cuda_integrated_gpus() {
    std::vector<CUDAIntegratedInfo> gpus;

    if (!initialize()) {
        return gpus;
    }

    int device_count;
    cudaError_t error = cudaGetDeviceCount(&device_count);
    if (error != cudaSuccess) {
        Logger::error("Failed to get CUDA device count: " + std::string(cudaGetErrorString(error)));
        return gpus;
    }

    Logger::info("Scanning " + std::to_string(device_count) + " CUDA devices for integrated GPUs...");

    for (int i = 0; i < device_count; i++) {
        cudaDeviceProp props;
        error = cudaGetDeviceProperties(&props, i);
        if (error != cudaSuccess) {
            Logger::warn("Failed to get properties for CUDA device " + std::to_string(i));
            continue;
        }

        // Check if this is an integrated GPU
        if (!is_integrated_gpu(i)) {
            continue;
        }

        CUDAIntegratedInfo gpu_info;
        gpu_info.device_id = i;
        gpu_info.name = props.name;
        gpu_info.type = identify_nvidia_integrated_type(props);
        
        // Compute capability
        std::stringstream cc_stream;
        cc_stream << props.major << "." << props.minor;
        gpu_info.compute_capability = cc_stream.str();

        // Memory information
        gpu_info.total_memory = props.totalGlobalMem;
        gpu_info.available_memory = props.totalGlobalMem * 0.8; // Conservative estimate
        gpu_info.shared_memory_per_block = props.sharedMemPerBlock;

        // Compute information
        gpu_info.multiprocessor_count = props.multiProcessorCount;
        gpu_info.max_threads_per_block = props.maxThreadsPerBlock;
        gpu_info.max_threads_per_multiprocessor = props.maxThreadsPerMultiProcessor;
        gpu_info.warp_size = props.warpSize;

        // Grid and block dimensions
        for (int j = 0; j < 3; j++) {
            gpu_info.max_grid_size[j] = props.maxGridSize[j];
            gpu_info.max_block_size[j] = props.maxThreadsDim[j];
        }

        // Features
        gpu_info.unified_memory_support = (props.unifiedAddressing == 1);
        gpu_info.is_integrated = props.integrated;
        gpu_info.is_power_constrained = detect_power_constraints(i);

        // Performance characteristics
        gpu_info.memory_bandwidth_gb_s = calculate_memory_bandwidth(props);
        gpu_info.memory_bus_width = props.memoryBusWidth;
        gpu_info.memory_clock_rate = props.memoryClockRate;
        gpu_info.gpu_clock_rate = props.clockRate;
        gpu_info.thermal_design_power = estimate_tdp(props, gpu_info.type);

        gpus.push_back(gpu_info);

        Logger::info("Found CUDA integrated GPU: " + gpu_info.name);
        Logger::debug("  Device ID: " + std::to_string(gpu_info.device_id));
        Logger::debug("  Compute Capability: " + gpu_info.compute_capability);
        Logger::debug("  Memory: " + std::to_string(gpu_info.total_memory / (1024*1024)) + " MB");
        Logger::debug("  Multiprocessors: " + std::to_string(gpu_info.multiprocessor_count));
        Logger::debug("  Unified Memory: " + (gpu_info.unified_memory_support ? "Yes" : "No"));
    }

    Logger::info("Found " + std::to_string(gpus.size()) + " CUDA integrated GPU(s)");
    return gpus;
}

std::unique_ptr<CUDAIntegratedInfo> CUDAIntegratedManager::get_best_cuda_integrated_gpu() {
    auto gpus = detect_cuda_integrated_gpus();
    
    if (gpus.empty()) {
        return nullptr;
    }

    // Sort by estimated performance
    std::sort(gpus.begin(), gpus.end(), [this](const CUDAIntegratedInfo& a, const CUDAIntegratedInfo& b) {
        return estimate_cuda_performance_ratio(a) > estimate_cuda_performance_ratio(b);
    });

    return std::make_unique<CUDAIntegratedInfo>(gpus[0]);
}

bool CUDAIntegratedManager::is_integrated_gpu(int device_id) {
    cudaDeviceProp props;
    cudaError_t error = cudaGetDeviceProperties(&props, device_id);
    if (error != cudaSuccess) {
        return false;
    }

    // Check CUDA integrated flag
    if (props.integrated) {
        return true;
    }

    // Check for Tegra devices (always integrated)
    if (is_tegra_device(props)) {
        return true;
    }

    // Check for GTX 1650 Ti and similar mid-range mobile GPUs
    std::string name = props.name;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (name.find("gtx") != std::string::npos && name.find("1650") != std::string::npos) {
        // GTX 1650 series - treat as integrated for optimization purposes
        // These are often found in laptops and benefit from integrated GPU optimizations
        return true;
    }

    // Check for mobile GPUs with shared memory architecture
    if (is_mobile_gpu(props) && has_unified_memory_architecture(props)) {
        return true;
    }

    // Check memory size - integrated GPUs typically have less dedicated memory
    // or share system memory. GTX 1650 Ti with 4GB is considered in this category
    size_t total_memory_gb = props.totalGlobalMem / (1024 * 1024 * 1024);
    if (total_memory_gb <= 4 && props.unifiedAddressing) {
        // Likely integrated or low-end mobile GPU
        return true;
    }

    return false;
}

NVIDIAIntegratedType CUDAIntegratedManager::identify_nvidia_integrated_type(const cudaDeviceProp& props) {
    std::string name = props.name;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    // Tegra identification
    if (name.find("tegra") != std::string::npos || name.find("jetson") != std::string::npos) {
        if (name.find("orin") != std::string::npos) {
            return NVIDIAIntegratedType::TEGRA_ORIN;
        } else if (name.find("xavier") != std::string::npos) {
            return NVIDIAIntegratedType::TEGRA_XAVIER;
        } else if (name.find("x2") != std::string::npos) {
            return NVIDIAIntegratedType::TEGRA_X2;
        } else if (name.find("x1") != std::string::npos) {
            return NVIDIAIntegratedType::TEGRA_X1;
        }
    }

    // GTX 1650 Ti identification (specific model)
    if (name.find("gtx") != std::string::npos && name.find("1650") != std::string::npos &&
        name.find("ti") != std::string::npos) {
        return NVIDIAIntegratedType::GTX_1650_TI;
    }

    // GTX 1650 series identification
    if (name.find("gtx") != std::string::npos && name.find("1650") != std::string::npos) {
        return NVIDIAIntegratedType::GTX_1650_SERIES;
    }

    // MX series identification
    if (name.find("mx") != std::string::npos) {
        return NVIDIAIntegratedType::LAPTOP_MX_SERIES;
    }

    // Mobile GTX identification
    if (name.find("gtx") != std::string::npos &&
        (name.find("mobile") != std::string::npos || name.find("m") != std::string::npos)) {
        return NVIDIAIntegratedType::LAPTOP_GTX_MOBILE;
    }

    // ARM-based integrated graphics
    if (props.integrated && (name.find("arm") != std::string::npos || 
                            props.totalGlobalMem < 2ULL * 1024 * 1024 * 1024)) {
        return NVIDIAIntegratedType::ARM_INTEGRATED;
    }

    return NVIDIAIntegratedType::UNKNOWN;
}

bool CUDAIntegratedManager::is_tegra_device(const cudaDeviceProp& props) {
    std::string name = props.name;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    
    return (name.find("tegra") != std::string::npos || 
            name.find("jetson") != std::string::npos ||
            name.find("shield") != std::string::npos);
}

bool CUDAIntegratedManager::is_mobile_gpu(const cudaDeviceProp& props) {
    std::string name = props.name;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    
    return (name.find("mobile") != std::string::npos ||
            name.find("mx") != std::string::npos ||
            name.find("max-q") != std::string::npos ||
            (name.find("gtx") != std::string::npos && name.find("m") != std::string::npos));
}

bool CUDAIntegratedManager::has_unified_memory_architecture(const cudaDeviceProp& props) {
    return (props.unifiedAddressing == 1 && props.integrated == 1);
}

float CUDAIntegratedManager::calculate_memory_bandwidth(const cudaDeviceProp& props) {
    // Memory bandwidth = (memory clock rate * 2) * (bus width / 8) / 1000
    // Factor of 2 for DDR, divide by 8 to convert bits to bytes, divide by 1000 for GB/s
    float bandwidth = (props.memoryClockRate * 2.0f * props.memoryBusWidth / 8.0f) / 1000000.0f;
    
    // For integrated GPUs, bandwidth is typically lower due to shared memory
    if (props.integrated) {
        bandwidth *= 0.6f; // Reduce by 40% for shared memory overhead
    }
    
    return bandwidth;
}

float CUDAIntegratedManager::estimate_tdp(const cudaDeviceProp& props, NVIDIAIntegratedType type) {
    switch (type) {
        case NVIDIAIntegratedType::TEGRA_X1:
            return 10.0f;
        case NVIDIAIntegratedType::TEGRA_X2:
            return 15.0f;
        case NVIDIAIntegratedType::TEGRA_XAVIER:
            return 20.0f;
        case NVIDIAIntegratedType::TEGRA_ORIN:
            return 25.0f;
        case NVIDIAIntegratedType::GTX_1650_TI:
            return 55.0f;  // Mobile variant TDP
        case NVIDIAIntegratedType::GTX_1650_SERIES:
            return 50.0f;  // Average for GTX 1650 series
        case NVIDIAIntegratedType::LAPTOP_MX_SERIES:
            return 25.0f;
        case NVIDIAIntegratedType::LAPTOP_GTX_MOBILE:
            return 35.0f;
        case NVIDIAIntegratedType::ARM_INTEGRATED:
            return 8.0f;
        default:
            // Estimate based on multiprocessor count and clock rate
            float base_tdp = props.multiProcessorCount * 2.0f;
            float clock_factor = props.clockRate / 1000000.0f; // Convert to GHz
            return base_tdp * clock_factor;
    }
}

bool CUDAIntegratedManager::detect_power_constraints(int device_id) {
    // For integrated GPUs, assume power constraints exist
    // This is a conservative approach for mobile/embedded devices
    return true;
}

float CUDAIntegratedManager::estimate_cuda_performance_ratio(const CUDAIntegratedInfo& gpu_info) {
    // Base performance on multiprocessor count and memory bandwidth
    float mp_score = gpu_info.multiprocessor_count / 32.0f; // Normalize to high-end discrete GPU
    float memory_score = gpu_info.memory_bandwidth_gb_s / 500.0f; // Normalize to high-end memory
    float clock_score = gpu_info.gpu_clock_rate / 2000000.0f; // Normalize to 2GHz

    // Integrated GPUs typically perform at 10-30% of discrete GPUs
    float base_ratio = (mp_score + memory_score + clock_score) / 3.0f;

    // Apply integrated GPU penalty
    base_ratio *= 0.25f; // 25% of equivalent discrete GPU

    // Type-specific adjustments
    switch (gpu_info.type) {
        case NVIDIAIntegratedType::TEGRA_ORIN:
            base_ratio *= 1.2f; // Best integrated performance
            break;
        case NVIDIAIntegratedType::TEGRA_XAVIER:
            base_ratio *= 1.0f;
            break;
        case NVIDIAIntegratedType::LAPTOP_GTX_MOBILE:
            base_ratio *= 0.8f;
            break;
        case NVIDIAIntegratedType::LAPTOP_MX_SERIES:
            base_ratio *= 0.6f;
            break;
        case NVIDIAIntegratedType::TEGRA_X2:
            base_ratio *= 0.5f;
            break;
        case NVIDIAIntegratedType::TEGRA_X1:
            base_ratio *= 0.4f;
            break;
        case NVIDIAIntegratedType::ARM_INTEGRATED:
            base_ratio *= 0.3f;
            break;
        default:
            base_ratio *= 0.5f;
            break;
    }

    return std::min(1.0f, std::max(0.1f, base_ratio));
}

void CUDAIntegratedManager::initialize_profiles() {
    profiles_.clear();
    profiles_.push_back(create_tegra_x1_profile());
    profiles_.push_back(create_tegra_x2_profile());
    profiles_.push_back(create_tegra_xavier_profile());
    profiles_.push_back(create_tegra_orin_profile());
    profiles_.push_back(create_gtx_1650_ti_profile());
    profiles_.push_back(create_gtx_1650_series_profile());
    profiles_.push_back(create_mx_series_profile());
    profiles_.push_back(create_mobile_gtx_profile());
    profiles_.push_back(create_arm_integrated_profile());
}

CUDAIntegratedProfile CUDAIntegratedManager::create_tegra_x1_profile() {
    CUDAIntegratedProfile profile;
    profile.name = "NVIDIA Tegra X1";
    profile.recommended_threads_per_block = 128;
    profile.recommended_blocks_per_grid = 64;
    profile.recommended_shared_memory_size = 16384; // 16KB
    profile.recommended_batch_size = 1000;
    profile.memory_usage_ratio = 0.6f;
    profile.enable_unified_memory = true;
    profile.enable_memory_pooling = true;
    profile.enable_thermal_throttling = true;
    profile.use_streams = true;
    profile.stream_count = 2;

    profile.kernel_parameters["max_registers_per_thread"] = 32;
    profile.kernel_parameters["occupancy_target"] = 50; // 50% occupancy target

    return profile;
}

CUDAIntegratedProfile CUDAIntegratedManager::create_tegra_orin_profile() {
    CUDAIntegratedProfile profile;
    profile.name = "NVIDIA Tegra Orin";
    profile.recommended_threads_per_block = 256;
    profile.recommended_blocks_per_grid = 128;
    profile.recommended_shared_memory_size = 32768; // 32KB
    profile.recommended_batch_size = 5000;
    profile.memory_usage_ratio = 0.7f;
    profile.enable_unified_memory = true;
    profile.enable_memory_pooling = true;
    profile.enable_thermal_throttling = true;
    profile.use_streams = true;
    profile.stream_count = 4;

    profile.kernel_parameters["max_registers_per_thread"] = 64;
    profile.kernel_parameters["occupancy_target"] = 75;

    return profile;
}

CUDAIntegratedProfile CUDAIntegratedManager::create_gtx_1650_ti_profile() {
    CUDAIntegratedProfile profile;
    profile.name = "NVIDIA GTX 1650 Ti";
    profile.recommended_threads_per_block = 256;  // Optimal for Turing architecture
    profile.recommended_blocks_per_grid = 512;    // 1024 CUDA cores / 2 for efficiency
    profile.recommended_shared_memory_size = 49152; // 48KB shared memory per SM
    profile.max_batch_size = 75000;               // Balanced for 4GB VRAM
    profile.memory_usage_ratio = 0.8f;            // 4GB GDDR6 - use 80%
    profile.enable_unified_memory = false;        // Discrete GPU
    profile.thermal_throttling_threshold = 83.0f; // Turing thermal limit
    profile.power_limit_watts = 55.0f;            // Mobile variant TDP
    profile.performance_scaling_factor = 1.2f;    // Mid-range discrete performance

    // GTX 1650 Ti specific kernel parameters
    profile.kernel_parameters["threads_per_warp"] = 32;
    profile.kernel_parameters["warps_per_sm"] = 32;  // 16 SMs * 2 warps
    profile.kernel_parameters["max_registers_per_thread"] = 255; // Turing register file
    profile.kernel_parameters["occupancy_target"] = 75; // Target 75% occupancy
    profile.kernel_parameters["memory_coalescing"] = 128; // 128-byte coalescing
    profile.kernel_parameters["cache_preference"] = 1; // Prefer L1 cache

    return profile;
}

CUDAIntegratedProfile CUDAIntegratedManager::create_gtx_1650_series_profile() {
    CUDAIntegratedProfile profile;
    profile.name = "NVIDIA GTX 1650 Series";
    profile.recommended_threads_per_block = 256;  // Optimal for Turing architecture
    profile.recommended_blocks_per_grid = 448;    // Slightly lower than Ti variant
    profile.recommended_shared_memory_size = 49152; // 48KB shared memory per SM
    profile.max_batch_size = 65000;               // Conservative for base model
    profile.memory_usage_ratio = 0.75f;           // 4GB GDDR6 - conservative usage
    profile.enable_unified_memory = false;        // Discrete GPU
    profile.thermal_throttling_threshold = 83.0f; // Turing thermal limit
    profile.power_limit_watts = 50.0f;            // Base model TDP
    profile.performance_scaling_factor = 1.1f;    // Slightly lower than Ti

    // GTX 1650 series specific kernel parameters
    profile.kernel_parameters["threads_per_warp"] = 32;
    profile.kernel_parameters["warps_per_sm"] = 28;  // 14 SMs * 2 warps
    profile.kernel_parameters["max_registers_per_thread"] = 255; // Turing register file
    profile.kernel_parameters["occupancy_target"] = 70; // Target 70% occupancy
    profile.kernel_parameters["memory_coalescing"] = 128; // 128-byte coalescing
    profile.kernel_parameters["cache_preference"] = 1; // Prefer L1 cache

    return profile;
}

void CUDAIntegratedManager::cleanup() {
    if (initialized_) {
        cudaDeviceReset();
        initialized_ = false;
        Logger::info("CUDA integrated GPU manager cleaned up");
    }
}

#endif // ENABLE_CUDA
