#include <gtest/gtest.h>
#include "core/config_manager.h"
#include <fstream>
#include <filesystem>

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        config = std::make_unique<ConfigManager>();
        test_config_file = "test_config.yaml";
    }

    void TearDown() override {
        // Clean up test files
        if (std::filesystem::exists(test_config_file)) {
            std::filesystem::remove(test_config_file);
        }
    }

    std::unique_ptr<ConfigManager> config;
    std::string test_config_file;
};

TEST_F(ConfigManagerTest, DefaultConfiguration) {
    // Test default values
    EXPECT_EQ(config->get_wallet_file(), "");
    EXPECT_EQ(config->get_charset(), "mixed");
    EXPECT_EQ(config->get_min_length(), 1);
    EXPECT_EQ(config->get_max_length(), 12);
    EXPECT_GT(config->get_threads(), 0);
    EXPECT_EQ(config->get_batch_size(), 10000);
    EXPECT_FALSE(config->get_use_gpu());
    EXPECT_EQ(config->get_log_level(), "info");
}

TEST_F(ConfigManagerTest, SettersAndGetters) {
    // Test wallet configuration
    config->set_wallet_file("/path/to/wallet.dat");
    EXPECT_EQ(config->get_wallet_file(), "/path/to/wallet.dat");
    
    config->set_wallet_type("bitcoin_core");
    EXPECT_EQ(config->get_wallet_type(), "bitcoin_core");
    
    // Test password generation configuration
    config->set_charset("lowercase");
    EXPECT_EQ(config->get_charset(), "lowercase");
    
    config->set_min_length(6);
    EXPECT_EQ(config->get_min_length(), 6);
    
    config->set_max_length(10);
    EXPECT_EQ(config->get_max_length(), 10);
    
    config->set_prefix("test");
    EXPECT_EQ(config->get_prefix(), "test");
    
    config->set_suffix("123");
    EXPECT_EQ(config->get_suffix(), "123");
    
    // Test performance configuration
    config->set_threads(8);
    EXPECT_EQ(config->get_threads(), 8);
    
    config->set_batch_size(5000);
    EXPECT_EQ(config->get_batch_size(), 5000);
    
    // Test GPU configuration
    config->set_use_gpu(true);
    EXPECT_TRUE(config->get_use_gpu());
    
    config->set_gpu_threads(1024);
    EXPECT_EQ(config->get_gpu_threads(), 1024);
}

TEST_F(ConfigManagerTest, ValidationValid) {
    // Set up valid configuration
    config->set_wallet_file("test_wallet.dat");
    config->set_min_length(6);
    config->set_max_length(12);
    config->set_threads(4);
    config->set_batch_size(1000);
    
    EXPECT_TRUE(config->is_valid());
    EXPECT_TRUE(config->get_validation_errors().empty());
}

TEST_F(ConfigManagerTest, ValidationInvalid) {
    // Test empty wallet file
    config->set_wallet_file("");
    EXPECT_FALSE(config->is_valid());
    
    auto errors = config->get_validation_errors();
    EXPECT_FALSE(errors.empty());
    EXPECT_TRUE(std::find_if(errors.begin(), errors.end(),
        [](const std::string& error) {
            return error.find("Wallet file is required") != std::string::npos;
        }) != errors.end());
    
    // Test invalid length range
    config->set_wallet_file("test.dat");
    config->set_min_length(10);
    config->set_max_length(5);
    
    EXPECT_FALSE(config->is_valid());
    errors = config->get_validation_errors();
    EXPECT_TRUE(std::find_if(errors.begin(), errors.end(),
        [](const std::string& error) {
            return error.find("Maximum length must be >= minimum length") != std::string::npos;
        }) != errors.end());
}

TEST_F(ConfigManagerTest, SaveAndLoadConfig) {
    // Set up configuration
    config->set_wallet_file("test_wallet.dat");
    config->set_charset("lowercase");
    config->set_min_length(8);
    config->set_max_length(16);
    config->set_threads(6);
    config->set_use_gpu(true);
    config->set_gpu_threads(512);
    
    // Save configuration
    EXPECT_TRUE(config->save_config(test_config_file));
    EXPECT_TRUE(std::filesystem::exists(test_config_file));
    
    // Create new config manager and load
    auto new_config = std::make_unique<ConfigManager>();
    EXPECT_TRUE(new_config->load_config(test_config_file));
    
    // Verify loaded values
    EXPECT_EQ(new_config->get_wallet_file(), "test_wallet.dat");
    EXPECT_EQ(new_config->get_charset(), "lowercase");
    EXPECT_EQ(new_config->get_min_length(), 8);
    EXPECT_EQ(new_config->get_max_length(), 16);
    EXPECT_EQ(new_config->get_threads(), 6);
    EXPECT_TRUE(new_config->get_use_gpu());
    EXPECT_EQ(new_config->get_gpu_threads(), 512);
}

TEST_F(ConfigManagerTest, RecoveryModes) {
    // Test recovery mode setting
    config->set_recovery_mode(ConfigManager::RecoveryMode::BRUTE_FORCE);
    EXPECT_EQ(config->get_recovery_mode(), ConfigManager::RecoveryMode::BRUTE_FORCE);
    
    config->set_recovery_mode(ConfigManager::RecoveryMode::DICTIONARY);
    EXPECT_EQ(config->get_recovery_mode(), ConfigManager::RecoveryMode::DICTIONARY);
    
    config->set_recovery_mode(ConfigManager::RecoveryMode::HYBRID);
    EXPECT_EQ(config->get_recovery_mode(), ConfigManager::RecoveryMode::HYBRID);
    
    config->set_recovery_mode(ConfigManager::RecoveryMode::GPU_ONLY);
    EXPECT_EQ(config->get_recovery_mode(), ConfigManager::RecoveryMode::GPU_ONLY);
}

TEST_F(ConfigManagerTest, ClusterConfiguration) {
    // Test cluster settings
    config->set_cluster_mode(true);
    EXPECT_TRUE(config->get_cluster_mode());
    
    config->set_cluster_node_id(2);
    EXPECT_EQ(config->get_cluster_node_id(), 2);
    
    config->set_cluster_total_nodes(5);
    EXPECT_EQ(config->get_cluster_total_nodes(), 5);
    
    // Test cluster validation
    config->set_wallet_file("test.dat");
    EXPECT_TRUE(config->is_valid());
    
    // Invalid cluster configuration
    config->set_cluster_node_id(5);  // >= total_nodes
    EXPECT_FALSE(config->is_valid());
    
    auto errors = config->get_validation_errors();
    EXPECT_TRUE(std::find_if(errors.begin(), errors.end(),
        [](const std::string& error) {
            return error.find("Cluster node ID must be between 0 and total_nodes-1") != std::string::npos;
        }) != errors.end());
}
