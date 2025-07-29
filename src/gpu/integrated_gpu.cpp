#include "gpu/integrated_gpu.h"
#include "utils/logger.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <regex>
#include <thread>

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <mach/mach.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <dxgi.h>
#endif

#ifdef __linux__
#include <sys/utsname.h>
#include <unistd.h>
#endif

#ifdef ENABLE_OPENCL
#include <CL/cl.h>
#endif

IntegratedGPUManager::IntegratedGPUManager() {
    initialize_profiles();
}

std::vector<IntegratedGPUInfo> IntegratedGPUManager::detect_integrated_gpus() {
    std::vector<IntegratedGPUInfo> gpus;
    
    Logger::info("Detecting integrated GPUs...");
    
    // Detect Intel GPUs
    auto intel_gpus = detect_intel_gpus();
    gpus.insert(gpus.end(), intel_gpus.begin(), intel_gpus.end());
    
    // Detect AMD GPUs
    auto amd_gpus = detect_amd_gpus();
    gpus.insert(gpus.end(), amd_gpus.begin(), amd_gpus.end());
    
    // Detect Apple GPUs
    auto apple_gpus = detect_apple_gpus();
    gpus.insert(gpus.end(), apple_gpus.begin(), apple_gpus.end());

    // Detect NVIDIA integrated GPUs (Tegra, mobile GPUs)
    auto nvidia_gpus = detect_nvidia_integrated_gpus();
    gpus.insert(gpus.end(), nvidia_gpus.begin(), nvidia_gpus.end());
    
    Logger::info("Found " + std::to_string(gpus.size()) + " integrated GPU(s)");
    
    for (const auto& gpu : gpus) {
        Logger::info("  - " + gpu.name + " (" + gpu.vendor + ")");
        Logger::debug("    Memory: " + std::to_string(gpu.total_memory / (1024*1024)) + " MB");
        Logger::debug("    Compute Units: " + std::to_string(gpu.compute_units));
    }
    
    return gpus;
}

std::unique_ptr<IntegratedGPUInfo> IntegratedGPUManager::get_best_integrated_gpu() {
    auto gpus = detect_integrated_gpus();
    
    if (gpus.empty()) {
        return nullptr;
    }
    
    // Sort by estimated performance
    std::sort(gpus.begin(), gpus.end(), [this](const IntegratedGPUInfo& a, const IntegratedGPUInfo& b) {
        return estimate_performance_ratio(a) > estimate_performance_ratio(b);
    });
    
    return std::make_unique<IntegratedGPUInfo>(gpus[0]);
}

std::vector<IntegratedGPUInfo> IntegratedGPUManager::detect_intel_gpus() {
    std::vector<IntegratedGPUInfo> gpus;
    
#ifdef ENABLE_OPENCL
    cl_platform_id platforms[16];
    cl_uint num_platforms;
    
    if (clGetPlatformIDs(16, platforms, &num_platforms) != CL_SUCCESS) {
        return gpus;
    }
    
    for (cl_uint i = 0; i < num_platforms; i++) {
        char vendor[256];
        if (clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, sizeof(vendor), vendor, nullptr) != CL_SUCCESS) {
            continue;
        }
        
        if (std::string(vendor).find("Intel") == std::string::npos) {
            continue;
        }
        
        cl_device_id devices[16];
        cl_uint num_devices;
        
        if (clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 16, devices, &num_devices) != CL_SUCCESS) {
            continue;
        }
        
        for (cl_uint j = 0; j < num_devices; j++) {
            IntegratedGPUInfo gpu_info;
            
            char device_name[256];
            clGetDeviceInfo(devices[j], CL_DEVICE_NAME, sizeof(device_name), device_name, nullptr);
            
            gpu_info.name = device_name;
            gpu_info.vendor = "Intel";
            gpu_info.type = identify_intel_gpu(device_name);
            
            cl_ulong memory_size;
            clGetDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(memory_size), &memory_size, nullptr);
            gpu_info.total_memory = memory_size;
            gpu_info.available_memory = memory_size * 0.8; // Conservative estimate
            
            cl_uint compute_units;
            clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, nullptr);
            gpu_info.compute_units = compute_units;
            
            size_t work_group_size;
            clGetDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(work_group_size), &work_group_size, nullptr);
            gpu_info.max_work_group_size = work_group_size;
            
            cl_uint clock_freq;
            clGetDeviceInfo(devices[j], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_freq), &clock_freq, nullptr);
            gpu_info.max_clock_frequency = clock_freq;
            
            gpu_info.supports_opencl = true;
            gpu_info.is_power_constrained = detect_laptop_system();
            gpu_info.shared_memory = get_system_memory() / 2; // Shared with system
            
            // Set TDP based on GPU type
            switch (gpu_info.type) {
                case IntegratedGPUType::INTEL_HD:
                    gpu_info.thermal_design_power = 15.0f;
                    break;
                case IntegratedGPUType::INTEL_IRIS:
                    gpu_info.thermal_design_power = 28.0f;
                    break;
                case IntegratedGPUType::INTEL_ARC:
                    gpu_info.thermal_design_power = 35.0f;
                    break;
                default:
                    gpu_info.thermal_design_power = 20.0f;
                    break;
            }
            
            gpus.push_back(gpu_info);
        }
    }
#endif
    
    return gpus;
}

std::vector<IntegratedGPUInfo> IntegratedGPUManager::detect_amd_gpus() {
    std::vector<IntegratedGPUInfo> gpus;
    
#ifdef ENABLE_OPENCL
    cl_platform_id platforms[16];
    cl_uint num_platforms;
    
    if (clGetPlatformIDs(16, platforms, &num_platforms) != CL_SUCCESS) {
        return gpus;
    }
    
    for (cl_uint i = 0; i < num_platforms; i++) {
        char vendor[256];
        if (clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, sizeof(vendor), vendor, nullptr) != CL_SUCCESS) {
            continue;
        }
        
        if (std::string(vendor).find("Advanced Micro Devices") == std::string::npos &&
            std::string(vendor).find("AMD") == std::string::npos) {
            continue;
        }
        
        cl_device_id devices[16];
        cl_uint num_devices;
        
        if (clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 16, devices, &num_devices) != CL_SUCCESS) {
            continue;
        }
        
        for (cl_uint j = 0; j < num_devices; j++) {
            IntegratedGPUInfo gpu_info;
            
            char device_name[256];
            clGetDeviceInfo(devices[j], CL_DEVICE_NAME, sizeof(device_name), device_name, nullptr);
            
            gpu_info.name = device_name;
            gpu_info.vendor = "AMD";
            gpu_info.type = identify_amd_gpu(device_name);
            
            cl_ulong memory_size;
            clGetDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(memory_size), &memory_size, nullptr);
            gpu_info.total_memory = memory_size;
            gpu_info.available_memory = memory_size * 0.8;
            
            cl_uint compute_units;
            clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, nullptr);
            gpu_info.compute_units = compute_units;
            
            size_t work_group_size;
            clGetDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(work_group_size), &work_group_size, nullptr);
            gpu_info.max_work_group_size = work_group_size;
            
            cl_uint clock_freq;
            clGetDeviceInfo(devices[j], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_freq), &clock_freq, nullptr);
            gpu_info.max_clock_frequency = clock_freq;
            
            gpu_info.supports_opencl = true;
            gpu_info.is_power_constrained = detect_laptop_system();
            gpu_info.shared_memory = get_system_memory() / 2;
            
            // Set TDP based on GPU type
            switch (gpu_info.type) {
                case IntegratedGPUType::AMD_VEGA:
                    gpu_info.thermal_design_power = 25.0f;
                    break;
                case IntegratedGPUType::AMD_RDNA:
                    gpu_info.thermal_design_power = 20.0f;
                    break;
                default:
                    gpu_info.thermal_design_power = 22.0f;
                    break;
            }
            
            gpus.push_back(gpu_info);
        }
    }
#endif
    
    return gpus;
}

std::vector<IntegratedGPUInfo> IntegratedGPUManager::detect_apple_gpus() {
    std::vector<IntegratedGPUInfo> gpus;
    
#ifdef __APPLE__
    // Apple Silicon detection
    size_t size = 0;
    if (sysctlbyname("machdep.cpu.brand_string", nullptr, &size, nullptr, 0) == 0) {
        std::vector<char> brand(size);
        if (sysctlbyname("machdep.cpu.brand_string", brand.data(), &size, nullptr, 0) == 0) {
            std::string cpu_brand(brand.data());
            
            if (cpu_brand.find("Apple") != std::string::npos) {
                IntegratedGPUInfo gpu_info;
                gpu_info.vendor = "Apple";
                gpu_info.supports_opencl = false; // Apple deprecated OpenCL
                gpu_info.supports_vulkan = false; // Use Metal instead
                gpu_info.is_power_constrained = true; // Always power constrained
                
                if (cpu_brand.find("M1") != std::string::npos) {
                    gpu_info.name = "Apple M1 GPU";
                    gpu_info.type = IntegratedGPUType::APPLE_M1;
                    gpu_info.compute_units = 8;
                    gpu_info.thermal_design_power = 10.0f;
                } else if (cpu_brand.find("M2") != std::string::npos) {
                    gpu_info.name = "Apple M2 GPU";
                    gpu_info.type = IntegratedGPUType::APPLE_M2;
                    gpu_info.compute_units = 10;
                    gpu_info.thermal_design_power = 12.0f;
                } else if (cpu_brand.find("M3") != std::string::npos) {
                    gpu_info.name = "Apple M3 GPU";
                    gpu_info.type = IntegratedGPUType::APPLE_M3;
                    gpu_info.compute_units = 12;
                    gpu_info.thermal_design_power = 15.0f;
                }
                
                // Get system memory (shared with GPU)
                int64_t memory_size;
                size = sizeof(memory_size);
                if (sysctlbyname("hw.memsize", &memory_size, &size, nullptr, 0) == 0) {
                    gpu_info.total_memory = memory_size / 2; // GPU gets half
                    gpu_info.available_memory = gpu_info.total_memory * 0.8;
                    gpu_info.shared_memory = memory_size;
                }
                
                gpu_info.max_work_group_size = 256; // Conservative estimate
                gpu_info.max_clock_frequency = 1000; // Approximate
                
                gpus.push_back(gpu_info);
            }
        }
    }
#endif
    
    return gpus;
}

std::vector<IntegratedGPUInfo> IntegratedGPUManager::detect_nvidia_integrated_gpus() {
    std::vector<IntegratedGPUInfo> gpus;

#ifdef ENABLE_CUDA
    #include "gpu/cuda_integrated.h"

    CUDAIntegratedManager cuda_manager;
    if (!cuda_manager.initialize()) {
        return gpus;
    }

    auto cuda_gpus = cuda_manager.detect_cuda_integrated_gpus();

    for (const auto& cuda_gpu : cuda_gpus) {
        IntegratedGPUInfo gpu_info;
        gpu_info.name = cuda_gpu.name;
        gpu_info.vendor = "NVIDIA";
        gpu_info.total_memory = cuda_gpu.total_memory;
        gpu_info.available_memory = cuda_gpu.available_memory;
        gpu_info.shared_memory = cuda_gpu.total_memory; // CUDA integrated GPUs use unified memory
        gpu_info.compute_units = cuda_gpu.multiprocessor_count;
        gpu_info.max_work_group_size = cuda_gpu.max_threads_per_block;
        gpu_info.max_clock_frequency = cuda_gpu.gpu_clock_rate / 1000; // Convert to MHz
        gpu_info.supports_opencl = false; // CUDA-only
        gpu_info.supports_vulkan = true;  // Most modern NVIDIA GPUs support Vulkan
        gpu_info.is_power_constrained = cuda_gpu.is_power_constrained;
        gpu_info.thermal_design_power = cuda_gpu.thermal_design_power;

        // Map CUDA integrated type to general integrated type
        switch (cuda_gpu.type) {
            case NVIDIAIntegratedType::TEGRA_X1:
            case NVIDIAIntegratedType::TEGRA_X2:
            case NVIDIAIntegratedType::TEGRA_XAVIER:
            case NVIDIAIntegratedType::TEGRA_ORIN:
                gpu_info.type = IntegratedGPUType::UNKNOWN; // Could add NVIDIA_TEGRA type
                break;
            case NVIDIAIntegratedType::LAPTOP_MX_SERIES:
            case NVIDIAIntegratedType::LAPTOP_GTX_MOBILE:
                gpu_info.type = IntegratedGPUType::UNKNOWN; // Could add NVIDIA_MOBILE type
                break;
            default:
                gpu_info.type = IntegratedGPUType::UNKNOWN;
                break;
        }

        gpus.push_back(gpu_info);
    }
#endif

    return gpus;
}

IntegratedGPUType IntegratedGPUManager::identify_intel_gpu(const std::string& device_name) {
    std::string name_lower = device_name;
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

    if (name_lower.find("arc") != std::string::npos) {
        return IntegratedGPUType::INTEL_ARC;
    } else if (name_lower.find("iris") != std::string::npos) {
        return IntegratedGPUType::INTEL_IRIS;
    } else if (name_lower.find("hd") != std::string::npos ||
               name_lower.find("uhd") != std::string::npos) {
        return IntegratedGPUType::INTEL_HD;
    }

    return IntegratedGPUType::UNKNOWN;
}

IntegratedGPUType IntegratedGPUManager::identify_amd_gpu(const std::string& device_name) {
    std::string name_lower = device_name;
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

    if (name_lower.find("rdna") != std::string::npos ||
        name_lower.find("6000") != std::string::npos ||
        name_lower.find("7000") != std::string::npos) {
        return IntegratedGPUType::AMD_RDNA;
    } else if (name_lower.find("vega") != std::string::npos) {
        return IntegratedGPUType::AMD_VEGA;
    }

    return IntegratedGPUType::UNKNOWN;
}

IntegratedGPUType IntegratedGPUManager::identify_apple_gpu(const std::string& device_name) {
    std::string name_lower = device_name;
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

    if (name_lower.find("m3") != std::string::npos) {
        return IntegratedGPUType::APPLE_M3;
    } else if (name_lower.find("m2") != std::string::npos) {
        return IntegratedGPUType::APPLE_M2;
    } else if (name_lower.find("m1") != std::string::npos) {
        return IntegratedGPUType::APPLE_M1;
    }

    return IntegratedGPUType::UNKNOWN;
}

bool IntegratedGPUManager::detect_laptop_system() {
#ifdef __linux__
    // Check for laptop indicators
    std::ifstream chassis_file("/sys/class/dmi/id/chassis_type");
    if (chassis_file.is_open()) {
        std::string chassis_type;
        std::getline(chassis_file, chassis_type);
        int type = std::stoi(chassis_type);
        // Types 8, 9, 10, 14 are laptop/notebook/handheld/sub-notebook
        return (type == 8 || type == 9 || type == 10 || type == 14);
    }

    // Check for battery presence
    return std::ifstream("/sys/class/power_supply/BAT0").good() ||
           std::ifstream("/sys/class/power_supply/BAT1").good();
#elif defined(_WIN32)
    // Check for battery on Windows
    SYSTEM_POWER_STATUS power_status;
    if (GetSystemPowerStatus(&power_status)) {
        return power_status.BatteryFlag != 128; // 128 = no battery
    }
#elif defined(__APPLE__)
    // Apple laptops always have batteries
    return true;
#endif

    return false; // Default to desktop
}

size_t IntegratedGPUManager::get_system_memory() {
#ifdef __linux__
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) {
            std::istringstream iss(line);
            std::string label, value, unit;
            iss >> label >> value >> unit;
            return std::stoull(value) * 1024; // Convert KB to bytes
        }
    }
#elif defined(_WIN32)
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    if (GlobalMemoryStatusEx(&mem_status)) {
        return mem_status.ullTotalPhys;
    }
#elif defined(__APPLE__)
    int64_t memory_size;
    size_t size = sizeof(memory_size);
    if (sysctlbyname("hw.memsize", &memory_size, &size, nullptr, 0) == 0) {
        return memory_size;
    }
#endif

    return 8ULL * 1024 * 1024 * 1024; // Default 8GB
}

int IntegratedGPUManager::get_cpu_core_count() {
    return std::thread::hardware_concurrency();
}

IntegratedGPUProfile IntegratedGPUManager::get_performance_profile(IntegratedGPUType type) {
    for (const auto& profile : profiles_) {
        if (profile.name.find("Intel HD") != std::string::npos && type == IntegratedGPUType::INTEL_HD) {
            return profile;
        } else if (profile.name.find("Intel Iris") != std::string::npos && type == IntegratedGPUType::INTEL_IRIS) {
            return profile;
        } else if (profile.name.find("Intel Arc") != std::string::npos && type == IntegratedGPUType::INTEL_ARC) {
            return profile;
        } else if (profile.name.find("AMD Vega") != std::string::npos && type == IntegratedGPUType::AMD_VEGA) {
            return profile;
        } else if (profile.name.find("AMD RDNA") != std::string::npos && type == IntegratedGPUType::AMD_RDNA) {
            return profile;
        } else if (profile.name.find("Apple M1") != std::string::npos && type == IntegratedGPUType::APPLE_M1) {
            return profile;
        } else if (profile.name.find("Apple M2") != std::string::npos && type == IntegratedGPUType::APPLE_M2) {
            return profile;
        } else if (profile.name.find("Apple M3") != std::string::npos && type == IntegratedGPUType::APPLE_M3) {
            return profile;
        }
    }

    // Return default profile
    IntegratedGPUProfile default_profile;
    default_profile.name = "Default";
    default_profile.recommended_work_group_size = 64;
    default_profile.recommended_batch_size = 1000;
    default_profile.memory_usage_ratio = 0.5f;
    default_profile.thread_count_multiplier = 4;
    default_profile.enable_memory_pooling = true;
    default_profile.enable_thermal_throttling = true;
    default_profile.compiler_options["-cl-fast-relaxed-math"] = "";

    return default_profile;
}
