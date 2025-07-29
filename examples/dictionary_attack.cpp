#include <iostream>
#include <memory>
#include <fstream>
#include "core/recovery_engine.h"
#include "core/config_manager.h"
#include "utils/logger.h"

/**
 * Dictionary attack example
 * 
 * This example demonstrates how to perform a dictionary-based
 * password recovery attack using common passwords and rules.
 */

void create_sample_dictionary() {
    // Create a sample dictionary file
    std::ofstream dict_file("sample_passwords.txt");
    if (dict_file.is_open()) {
        dict_file << "password\n";
        dict_file << "123456\n";
        dict_file << "password123\n";
        dict_file << "admin\n";
        dict_file << "letmein\n";
        dict_file << "welcome\n";
        dict_file << "monkey\n";
        dict_file << "dragon\n";
        dict_file << "qwerty\n";
        dict_file << "bitcoin\n";
        dict_file << "wallet\n";
        dict_file << "secret\n";
        dict_file << "mypassword\n";
        dict_file << "test123\n";
        dict_file << "password1\n";
        dict_file.close();
        Logger::info("Created sample dictionary: sample_passwords.txt");
    }
}

void create_sample_rules() {
    // Create a sample rules file
    std::ofstream rules_file("sample_rules.txt");
    if (rules_file.is_open()) {
        rules_file << "# Password transformation rules\n";
        rules_file << "# Format: rule_name:pattern:transformations\n";
        rules_file << "\n";
        rules_file << "# Append numbers\n";
        rules_file << "append_digits:$word:$0,$1,$2,$3,$4,$5,$6,$7,$8,$9\n";
        rules_file << "append_years:$word:$2020,$2021,$2022,$2023,$2024\n";
        rules_file << "\n";
        rules_file << "# Prepend numbers\n";
        rules_file << "prepend_digits:$word:0$,1$,2$,3$,4$,5$,6$,7$,8$,9$\n";
        rules_file << "\n";
        rules_file << "# Capitalize variations\n";
        rules_file << "capitalize:$word:c\n";
        rules_file << "uppercase:$word:u\n";
        rules_file << "lowercase:$word:l\n";
        rules_file << "\n";
        rules_file << "# Common substitutions\n";
        rules_file << "leet_speak:$word:sa@,so0,si1,se3,st7\n";
        rules_file << "\n";
        rules_file << "# Append common suffixes\n";
        rules_file << "common_suffix:$word:$!,$123,$!@#,$_1\n";
        rules_file.close();
        Logger::info("Created sample rules: sample_rules.txt");
    }
}

int main() {
    // Initialize logger
    Logger::initialize("info", true);
    Logger::info("Starting dictionary attack example");

    try {
        // Create sample files
        create_sample_dictionary();
        create_sample_rules();
        
        // Create configuration
        auto config = std::make_shared<ConfigManager>();
        
        // Set dictionary attack configuration
        config->set_wallet_file("example_wallet.dat");
        config->set_recovery_mode(ConfigManager::RecoveryMode::DICTIONARY);
        config->set_dictionary_file("sample_passwords.txt");
        config->set_rules_file("sample_rules.txt");
        config->set_threads(4);
        config->set_batch_size(500);
        
        // Optional: Set additional constraints
        config->set_min_length(4);
        config->set_max_length(20);
        config->set_prefix("");  // No prefix constraint
        config->set_suffix("");  // No suffix constraint
        
        // Validate configuration
        if (!config->is_valid()) {
            Logger::error("Configuration validation failed:");
            for (const auto& error : config->get_validation_errors()) {
                Logger::error("  - " + error);
            }
            return 1;
        }
        
        // Print configuration summary
        Logger::info("Dictionary attack configuration:");
        Logger::info("  Wallet file: " + config->get_wallet_file());
        Logger::info("  Dictionary file: " + config->get_dictionary_file());
        Logger::info("  Rules file: " + config->get_rules_file());
        Logger::info("  Threads: " + std::to_string(config->get_threads()));
        Logger::info("  Batch size: " + std::to_string(config->get_batch_size()));
        
        // Create recovery engine
        RecoveryEngine engine(config);
        
        // Start recovery
        Logger::info("Starting dictionary-based password recovery...");
        auto start_time = std::chrono::high_resolution_clock::now();
        
        bool success = engine.run();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        
        if (success) {
            Logger::info("Password recovery completed successfully!");
            
            // Get final statistics
            auto stats = engine.get_stats();
            Logger::info("Recovery statistics:");
            Logger::info("  Total passwords tested: " + std::to_string(stats.passwords_tested));
            Logger::info("  Average speed: " + std::to_string(stats.passwords_per_second) + " passwords/sec");
            Logger::info("  Total time: " + std::to_string(duration.count()) + " seconds");
            Logger::info("  Progress: " + std::to_string(stats.progress_percentage) + "%");
        } else {
            Logger::info("Password not found in dictionary");
            Logger::info("Consider:");
            Logger::info("  - Using a larger dictionary");
            Logger::info("  - Adding more transformation rules");
            Logger::info("  - Trying a hybrid approach (dictionary + brute force)");
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
 * g++ -std=c++17 -I../include dictionary_attack.cpp -o dictionary_attack -lssl -lcrypto -lpthread
 * 
 * Usage:
 * ./dictionary_attack
 * 
 * This will create sample dictionary and rules files, then attempt recovery.
 * In a real scenario, you would use comprehensive password dictionaries
 * such as:
 * - rockyou.txt
 * - SecLists password collections
 * - Custom dictionaries based on target information
 */
