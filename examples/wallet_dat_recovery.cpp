#include <iostream>
#include <memory>
#include <chrono>
#include <iomanip>
#include "wallets/bitcoin_core_wallet.h"
#include "core/recovery_engine.h"
#include "core/config_manager.h"
#include "utils/logger.h"

/**
 * Bitcoin Core wallet.dat recovery example
 * 
 * This example demonstrates how to recover Bitcoin Core wallet.dat files
 * without requiring a full blockchain download, using blockchain APIs
 * to check balances and transaction history.
 */

void print_wallet_info(const std::string& wallet_file) {
    Logger::info("=== Wallet File Analysis ===");
    
    BitcoinCoreWallet wallet(wallet_file);
    
    if (!wallet.load()) {
        Logger::error("Failed to load wallet file: " + wallet.get_last_error());
        return;
    }
    
    if (!wallet.is_valid()) {
        Logger::error("Invalid wallet file format");
        return;
    }
    
    auto metadata = wallet.get_metadata();
    Logger::info("Wallet Format: Bitcoin Core");
    Logger::info("Encryption: AES-256-CBC");
    Logger::info("Key Derivation Iterations: " + std::to_string(metadata.iterations));
    Logger::info("Estimated test time: " + std::to_string(wallet.get_estimated_test_time()) + " microseconds");
    Logger::info("Wallet appears to be valid and encrypted");
}

void demonstrate_password_recovery(const std::string& wallet_file) {
    Logger::info("=== Password Recovery Demo ===");
    
    try {
        BitcoinCoreWallet wallet(wallet_file);
        
        if (!wallet.load()) {
            Logger::error("Failed to load wallet: " + wallet.get_last_error());
            return;
        }
        
        // Test some common passwords (in real usage, use proper password generation)
        std::vector<std::string> test_passwords = {
            "password",
            "123456",
            "password123",
            "bitcoin",
            "wallet",
            "mypassword",
            "test123"
        };
        
        Logger::info("Testing " + std::to_string(test_passwords.size()) + " common passwords...");
        
        for (const auto& password : test_passwords) {
            Logger::info("Testing password: " + password);
            
            if (wallet.test_password(password)) {
                Logger::info("SUCCESS! Password found: " + password);
                
                // Perform full recovery
                auto result = wallet.recover_wallet(password);
                
                if (result.success) {
                    Logger::info("Wallet recovery completed successfully!");
                    Logger::info("Total addresses found: " + std::to_string(result.total_addresses));
                    Logger::info("Addresses with funds: " + std::to_string(result.funded_addresses));
                    Logger::info("Total balance: " + std::to_string(result.total_balance_satoshis / 100000000.0) + " BTC");
                    
                    // Export results
                    wallet.export_to_text(result.private_keys, "recovery_results.txt");
                    wallet.export_to_json(result.private_keys, "recovery_results.json");
                    wallet.export_to_csv(result.private_keys, "recovery_results.csv");
                    wallet.export_to_electrum(result.private_keys, "electrum_import.json");
                    
                    Logger::info("Recovery results exported to multiple formats");
                    
                    // Show funded addresses
                    if (result.funded_addresses > 0) {
                        Logger::info("Addresses with funds:");
                        for (const auto& key : result.private_keys) {
                            if (key.has_balance) {
                                Logger::info("  " + key.address + ": " + 
                                           std::to_string(key.balance_satoshis / 100000000.0) + " BTC");
                            }
                        }
                    }
                }
                return;
            }
        }
        
        Logger::info("None of the test passwords worked. In real usage, use:");
        Logger::info("1. Dictionary attacks with comprehensive wordlists");
        Logger::info("2. Brute force attacks with known password patterns");
        Logger::info("3. GPU acceleration for faster testing");
        
    } catch (const std::exception& e) {
        Logger::error("Error during recovery: " + std::string(e.what()));
    }
}

void demonstrate_api_configuration() {
    Logger::info("=== Blockchain API Configuration ===");
    
    BitcoinCoreWallet wallet("example_wallet.dat");
    
    // Configure API keys (optional but recommended for higher rate limits)
    Logger::info("Configuring blockchain API services...");
    
    // BlockCypher API (requires free registration)
    wallet.set_api_key("blockcypher", "your-blockcypher-api-key-here");
    
    // Custom API endpoints (if needed)
    wallet.set_api_endpoint("blockstream", "https://blockstream.info/api");
    wallet.set_api_endpoint("blockchair", "https://api.blockchair.com/bitcoin");
    
    // Enable testnet mode for testing
    wallet.enable_testnet(false); // Set to true for testnet
    
    Logger::info("API configuration completed");
    Logger::info("Available services:");
    Logger::info("  - Blockstream.info (no API key required)");
    Logger::info("  - Blockchair.com (no API key required, rate limited)");
    Logger::info("  - BlockCypher.com (API key recommended for higher limits)");
    Logger::info("");
    Logger::info("To get API keys:");
    Logger::info("  - BlockCypher: https://www.blockcypher.com/dev/");
    Logger::info("  - Other services may require registration for higher rate limits");
}

void show_recovery_workflow() {
    Logger::info("=== Complete Recovery Workflow ===");
    Logger::info("");
    Logger::info("1. PREPARATION:");
    Logger::info("   - Locate your wallet.dat file (usually in Bitcoin data directory)");
    Logger::info("   - Backup the wallet.dat file before attempting recovery");
    Logger::info("   - Ensure you have network connectivity for balance checking");
    Logger::info("");
    Logger::info("2. PASSWORD RECOVERY:");
    Logger::info("   - Start with known password variations");
    Logger::info("   - Use dictionary attacks with common passwords");
    Logger::info("   - Try brute force for short passwords");
    Logger::info("   - Use GPU acceleration for faster processing");
    Logger::info("");
    Logger::info("3. KEY EXTRACTION:");
    Logger::info("   - Once password is found, extract all private keys");
    Logger::info("   - Generate both compressed and uncompressed addresses");
    Logger::info("   - Export keys in multiple formats (WIF, hex, etc.)");
    Logger::info("");
    Logger::info("4. BALANCE VERIFICATION:");
    Logger::info("   - Check all addresses for current balances");
    Logger::info("   - Query transaction history");
    Logger::info("   - Identify addresses with funds");
    Logger::info("");
    Logger::info("5. FUND RECOVERY:");
    Logger::info("   - Import private keys into a modern wallet (Electrum, etc.)");
    Logger::info("   - Verify balances in the new wallet");
    Logger::info("   - Transfer funds to a new, secure wallet");
    Logger::info("");
    Logger::info("6. SECURITY:");
    Logger::info("   - Securely delete recovery files after use");
    Logger::info("   - Never share private keys or recovery files");
    Logger::info("   - Use the recovered funds immediately");
}

void show_import_instructions() {
    Logger::info("=== Importing Recovered Keys ===");
    Logger::info("");
    Logger::info("ELECTRUM WALLET:");
    Logger::info("1. Open Electrum wallet");
    Logger::info("2. File -> New/Restore");
    Logger::info("3. Choose 'Import Bitcoin addresses or private keys'");
    Logger::info("4. Paste WIF private keys (one per line) or import JSON file");
    Logger::info("5. Electrum will automatically check balances");
    Logger::info("");
    Logger::info("BITCOIN CORE:");
    Logger::info("1. Open Bitcoin Core (requires full blockchain sync)");
    Logger::info("2. Use 'importprivkey' command in console");
    Logger::info("3. Example: importprivkey \"your-wif-key-here\" \"label\"");
    Logger::info("");
    Logger::info("OTHER WALLETS:");
    Logger::info("- Most wallets support WIF private key import");
    Logger::info("- Some support JSON import files");
    Logger::info("- Always verify balances after import");
    Logger::info("");
    Logger::info("SECURITY NOTES:");
    Logger::info("- Import keys into a wallet on an offline computer first");
    Logger::info("- Verify balances before going online");
    Logger::info("- Transfer funds to a new wallet with a new seed phrase");
    Logger::info("- Delete all recovery files securely");
}

int main(int argc, char* argv[]) {
    // Initialize logger
    Logger::initialize("info", true);
    Logger::info("Bitcoin Core wallet.dat Recovery Example");
    Logger::info("========================================");
    
    if (argc < 2) {
        Logger::info("Usage: " + std::string(argv[0]) + " <wallet.dat>");
        Logger::info("");
        Logger::info("This example demonstrates:");
        Logger::info("1. Wallet.dat file analysis");
        Logger::info("2. Password recovery without blockchain download");
        Logger::info("3. Private key extraction");
        Logger::info("4. Balance checking via blockchain APIs");
        Logger::info("5. Export to multiple formats");
        Logger::info("");
        
        // Show workflow and configuration info
        demonstrate_api_configuration();
        show_recovery_workflow();
        show_import_instructions();
        
        return 1;
    }
    
    std::string wallet_file = argv[1];
    
    try {
        // Analyze wallet file
        print_wallet_info(wallet_file);
        
        // Demonstrate password recovery
        demonstrate_password_recovery(wallet_file);
        
        Logger::info("");
        Logger::info("=== Recovery Complete ===");
        Logger::info("Check the generated files:");
        Logger::info("- recovery_results.txt (human-readable format)");
        Logger::info("- recovery_results.json (structured data)");
        Logger::info("- recovery_results.csv (spreadsheet format)");
        Logger::info("- electrum_import.json (Electrum wallet import)");
        
    } catch (const std::exception& e) {
        Logger::error("Error: " + std::string(e.what()));
        return 1;
    }
    
    Logger::shutdown();
    return 0;
}

/*
 * Compilation:
 * g++ -std=c++17 -I../include wallet_dat_recovery.cpp -o wallet_dat_recovery \
 *     -lssl -lcrypto -lpthread -lcurl -ljsoncpp
 * 
 * Usage:
 * ./wallet_dat_recovery /path/to/wallet.dat
 * 
 * Prerequisites:
 * - libssl-dev (OpenSSL)
 * - libcurl4-openssl-dev (HTTP requests)
 * - libjsoncpp-dev (JSON parsing)
 * 
 * This example will:
 * 1. Analyze the wallet.dat file structure
 * 2. Attempt password recovery with common passwords
 * 3. Extract private keys if password is found
 * 4. Check balances using blockchain APIs
 * 5. Export results in multiple formats
 * 6. Provide instructions for importing keys into other wallets
 */
