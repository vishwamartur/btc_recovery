#include <iostream>
#include <memory>
#include <chrono>
#include "core/recovery_engine.h"
#include "core/config_manager.h"
#include "gpu/integrated_gpu.h"
#ifdef ENABLE_CUDA
#include "gpu/cuda_integrated.h"
#endif
#include "utils/logger.h"

/**
 * Integrated GPU Bitcoin wallet recovery example
 * 
 * This example demonstrates how to use the recovery engine
 * with integrated graphics cards for password recovery.
 */

void print_integrated_gpu_info() {
    Logger::info("=== Integrated GPU Detection ===");
    
    IntegratedGPUManager gpu_manager;
    auto gpus = gpu_manager.detect_integrated_gpus();
    
    if (gpus.empty()) {
        Logger::warn("No integrated GPUs detected");
        return;
    }
    
    for (const auto& gpu : gpus) {
        Logger::info("Found integrated GPU: " + gpu.name);
        Logger::info("  Vendor: " + gpu.vendor);
        Logger::info("  Memory: " + std::to_string(gpu.total_memory / (1024*1024)) + " MB");
        Logger::info("  Compute Units: " + std::to_string(gpu.compute_units));
        Logger::info("  Max Work Group Size: " + std::to_string(gpu.max_work_group_size));
        Logger::info("  OpenCL Support: " + (gpu.supports_opencl ? "Yes" : "No"));
        Logger::info("  Power Constrained: " + (gpu.is_power_constrained ? "Yes" : "No"));
        Logger::info("  TDP: " + std::to_string(gpu.thermal_design_power) + "W");
        
        // Get performance profile
        auto profile = gpu_manager.get_performance_profile(gpu.type);
        Logger::info("  Recommended Work Group Size: " + std::to_string(profile.recommended_work_group_size));
        Logger::info("  Recommended Batch Size: " + std::to_string(profile.recommended_batch_size));
        Logger::info("  Memory Usage Ratio: " + std::to_string(profile.memory_usage_ratio));
        Logger::info("");
    }
}

#ifdef ENABLE_CUDA
void print_cuda_integrated_gpu_info() {
    Logger::info("=== CUDA Integrated GPU Detection ===");
    
    CUDAIntegratedManager cuda_manager;
    if (!cuda_manager.initialize()) {
        Logger::warn("CUDA integrated GPU manager initialization failed");
        return;
    }
    
    auto cuda_gpus = cuda_manager.detect_cuda_integrated_gpus();
    
    if (cuda_gpus.empty()) {
        Logger::warn("No CUDA integrated GPUs detected");
        return;
    }
    
    for (const auto& gpu : cuda_gpus) {
        Logger::info("Found CUDA integrated GPU: " + gpu.name);
        Logger::info("  Device ID: " + std::to_string(gpu.device_id));
        Logger::info("  Compute Capability: " + gpu.compute_capability);
        Logger::info("  Memory: " + std::to_string(gpu.total_memory / (1024*1024)) + " MB");
        Logger::info("  Multiprocessors: " + std::to_string(gpu.multiprocessor_count));
        Logger::info("  Max Threads per Block: " + std::to_string(gpu.max_threads_per_block));
        Logger::info("  Unified Memory: " + (gpu.unified_memory_support ? "Yes" : "No"));
        Logger::info("  Power Constrained: " + (gpu.is_power_constrained ? "Yes" : "No"));
        Logger::info("  Memory Bandwidth: " + std::to_string(gpu.memory_bandwidth_gb_s) + " GB/s");
        Logger::info("  TDP: " + std::to_string(gpu.thermal_design_power) + "W");
        
        // Get performance profile
        auto profile = cuda_manager.get_performance_profile(gpu);
        Logger::info("  Recommended Threads per Block: " + std::to_string(profile.recommended_threads_per_block));
        Logger::info("  Recommended Blocks per Grid: " + std::to_string(profile.recommended_blocks_per_grid));
        Logger::info("  Recommended Batch Size: " + std::to_string(profile.recommended_batch_size));
        Logger::info("  Enable Unified Memory: " + (profile.enable_unified_memory ? "Yes" : "No"));
        Logger::info("");
    }
}
#endif

void demonstrate_integrated_gpu_recovery() {
    Logger::info("=== Integrated GPU Recovery Demo ===");
    
    try {
        // Create configuration optimized for integrated graphics
        auto config = std::make_shared<ConfigManager>();
        
        // Set basic configuration
        config->set_wallet_file("example_wallet.dat");
        config->set_charset("lowercase");
        config->set_min_length(4);
        config->set_max_length(6);
        config->set_use_gpu(true);
        
        // Optimize for integrated graphics
        config->set_threads(4); // Fewer threads to leave resources for GPU
        config->set_batch_size(1000); // Smaller batches for limited memory
        config->set_gpu_threads(128); // Conservative GPU thread count
        
        // Validate configuration
        if (!config->is_valid()) {
            Logger::error("Configuration validation failed:");
            for (const auto& error : config->get_validation_errors()) {
                Logger::error("  - " + error);
            }
            return;
        }
        
        Logger::info("Configuration for integrated GPU recovery:");
        Logger::info("  Wallet file: " + config->get_wallet_file());
        Logger::info("  Character set: " + config->get_charset());
        Logger::info("  Password length: " + std::to_string(config->get_min_length()) + 
                    "-" + std::to_string(config->get_max_length()));
        Logger::info("  CPU threads: " + std::to_string(config->get_threads()));
        Logger::info("  Batch size: " + std::to_string(config->get_batch_size()));
        Logger::info("  GPU threads: " + std::to_string(config->get_gpu_threads()));
        
        // Create recovery engine
        RecoveryEngine engine(config);
        
        // Start recovery with timing
        Logger::info("Starting integrated GPU password recovery...");
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Note: This would normally run the actual recovery
        // For demo purposes, we'll simulate a short run
        Logger::info("Simulating recovery process...");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        Logger::info("Demo recovery completed in " + std::to_string(duration.count()) + " ms");
        
        // Show performance recommendations
        Logger::info("");
        Logger::info("=== Performance Recommendations ===");
        
        IntegratedGPUManager gpu_manager;
        auto best_gpu = gpu_manager.get_best_integrated_gpu();
        
        if (best_gpu) {
            Logger::info("Best integrated GPU: " + best_gpu->name);
            
            if (best_gpu->is_power_constrained) {
                Logger::info("Power-constrained device detected. Recommendations:");
                Logger::info("  - Use smaller batch sizes (500-2000)");
                Logger::info("  - Enable thermal throttling");
                Logger::info("  - Consider CPU-only mode for very long runs");
                Logger::info("  - Monitor system temperature");
            }
            
            if (best_gpu->total_memory < 2ULL * 1024 * 1024 * 1024) { // Less than 2GB
                Logger::info("Limited memory detected. Recommendations:");
                Logger::info("  - Reduce buffer sizes");
                Logger::info("  - Use smaller work group sizes");
                Logger::info("  - Enable memory pooling");
            }
            
            float performance_ratio = gpu_manager.estimate_performance_ratio(*best_gpu);
            Logger::info("Estimated performance vs discrete GPU: " + 
                        std::to_string(int(performance_ratio * 100)) + "%");
            
            if (performance_ratio < 0.3f) {
                Logger::info("Low GPU performance detected. Consider:");
                Logger::info("  - Hybrid CPU+GPU approach");
                Logger::info("  - Focus on CPU optimization");
                Logger::info("  - Use dictionary attacks instead of brute force");
            }
        }
        
    } catch (const std::exception& e) {
        Logger::error("Error in integrated GPU recovery demo: " + std::string(e.what()));
    }
}

int main() {
    // Initialize logger
    Logger::initialize("info", true);
    Logger::info("Starting integrated GPU recovery example");
    
    try {
        // Print system information
        print_integrated_gpu_info();
        
#ifdef ENABLE_CUDA
        print_cuda_integrated_gpu_info();
#endif
        
        // Demonstrate recovery with integrated graphics
        demonstrate_integrated_gpu_recovery();
        
        Logger::info("");
        Logger::info("=== Usage Tips for Integrated Graphics ===");
        Logger::info("1. Integrated GPUs share system memory - monitor total memory usage");
        Logger::info("2. Thermal throttling is common - enable temperature monitoring");
        Logger::info("3. Power efficiency is important on laptops - use appropriate presets");
        Logger::info("4. Consider hybrid CPU+GPU approach for best performance");
        Logger::info("5. Dictionary attacks may be more efficient than brute force");
        Logger::info("6. Use smaller batch sizes to avoid memory pressure");
        Logger::info("7. Enable unified memory on supported NVIDIA integrated GPUs");
        Logger::info("8. For Apple Silicon, consider CPU-only mode due to limited GPU compute support");
        
    } catch (const std::exception& e) {
        Logger::error("Error: " + std::string(e.what()));
        return 1;
    }
    
    Logger::shutdown();
    return 0;
}

/*
 * Compilation:
 * g++ -std=c++17 -I../include integrated_gpu_recovery.cpp -o integrated_gpu_recovery -lssl -lcrypto -lpthread
 * 
 * With CUDA support:
 * nvcc -std=c++17 -I../include integrated_gpu_recovery.cpp -o integrated_gpu_recovery -lssl -lcrypto -lpthread -lcuda -lcudart
 * 
 * Usage:
 * ./integrated_gpu_recovery
 * 
 * This example will:
 * 1. Detect available integrated GPUs
 * 2. Show their capabilities and recommended settings
 * 3. Demonstrate configuration for integrated graphics
 * 4. Provide performance recommendations
 * 5. Show usage tips for different integrated GPU types
 */
