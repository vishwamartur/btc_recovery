#include <iostream>
#include <memory>
#include "core/recovery_engine.h"
#include "core/config_manager.h"
#include "utils/logger.h"

/**
 * Basic Bitcoin wallet recovery example
 * 
 * This example demonstrates how to use the recovery engine
 * for basic password recovery operations.
 */

int main() {
    // Initialize logger
    Logger::initialize("info", true);
    Logger::info("Starting basic recovery example");

    try {
        // Create configuration
        auto config = std::make_shared<ConfigManager>();
        
        // Set basic configuration
        config->set_wallet_file("example_wallet.dat");
        config->set_charset("lowercase");
        config->set_min_length(6);
        config->set_max_length(8);
        config->set_threads(4);
        config->set_batch_size(1000);
        
        // Validate configuration
        if (!config->is_valid()) {
            Logger::error("Configuration validation failed:");
            for (const auto& error : config->get_validation_errors()) {
                Logger::error("  - " + error);
            }
            return 1;
        }
        
        // Create recovery engine
        RecoveryEngine engine(config);
        
        // Start recovery
        Logger::info("Starting password recovery...");
        bool success = engine.run();
        
        if (success) {
            Logger::info("Password recovery completed successfully!");
            
            // Get final statistics
            auto stats = engine.get_stats();
            Logger::info("Total passwords tested: " + std::to_string(stats.passwords_tested));
            Logger::info("Average speed: " + std::to_string(stats.passwords_per_second) + " passwords/sec");
            Logger::info("Total time: " + std::to_string(stats.elapsed_time.count()) + " seconds");
        } else {
            Logger::info("Password not found in the specified search space");
        }
        
    } catch (const std::exception& e) {
        Logger::error("Error: " + std::string(e.what()));
        return 1;
    }
    
    Logger::shutdown();
    return 0;
}

/*
 * Compilation:
 * g++ -std=c++17 -I../include basic_recovery.cpp -o basic_recovery -lssl -lcrypto -lpthread
 * 
 * Usage:
 * ./basic_recovery
 */
