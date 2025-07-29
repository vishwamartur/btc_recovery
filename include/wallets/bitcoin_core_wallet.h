#pragma once

#include "wallet_base.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <openssl/evp.h>
#include <openssl/aes.h>

/**
 * Bitcoin Core wallet.dat recovery without blockchain download
 * Based on BitPay recovery methodology
 */

/**
 * Private key information structure
 */
struct PrivateKeyInfo {
    std::string address;           // Bitcoin address
    std::string private_key_hex;   // Private key in hex format
    std::string private_key_wif;   // Private key in WIF format
    std::string public_key_hex;    // Public key in hex format
    bool compressed;               // Whether the key uses compressed format
    std::string label;             // Address label (if any)
    uint64_t balance_satoshis;     // Balance in satoshis (from API)
    int transaction_count;         // Number of transactions
    bool has_balance;              // Whether address has funds
};

/**
 * Wallet recovery result structure
 */
struct WalletRecoveryResult {
    bool success;
    std::string password;
    std::vector<PrivateKeyInfo> private_keys;
    std::string master_key_hex;
    uint64_t total_balance_satoshis;
    int total_addresses;
    int funded_addresses;
    std::string recovery_timestamp;
    std::string wallet_version;
};

/**
 * Bitcoin Core wallet.dat handler
 */
class BitcoinCoreWallet : public WalletBase {
public:
    explicit BitcoinCoreWallet(const std::string& wallet_file);
    ~BitcoinCoreWallet() override = default;

    // WalletBase interface implementation
    bool load() override;
    bool test_password(const std::string& password) override;
    WalletMetadata get_metadata() const override;
    bool is_valid() const override;
    WalletFormat get_format() const override;
    EncryptionType get_encryption_type() const override;
    uint64_t get_estimated_test_time() const override;

    // Bitcoin Core specific functionality
    /**
     * Recover wallet with password and extract private keys
     * @param password The wallet password
     * @return WalletRecoveryResult with all extracted information
     */
    WalletRecoveryResult recover_wallet(const std::string& password);

    /**
     * Extract private keys from decrypted wallet
     * @param password The correct wallet password
     * @return vector of private key information
     */
    std::vector<PrivateKeyInfo> extract_private_keys(const std::string& password);

    /**
     * Check balances for all addresses using blockchain APIs
     * @param private_keys vector of private key info to update
     * @return true if balance check was successful
     */
    bool check_balances(std::vector<PrivateKeyInfo>& private_keys);

    /**
     * Export private keys in various formats
     */
    bool export_to_text(const std::vector<PrivateKeyInfo>& keys, const std::string& filename);
    bool export_to_json(const std::vector<PrivateKeyInfo>& keys, const std::string& filename);
    bool export_to_csv(const std::vector<PrivateKeyInfo>& keys, const std::string& filename);
    bool export_to_electrum(const std::vector<PrivateKeyInfo>& keys, const std::string& filename);

    /**
     * Configuration for blockchain API services
     */
    void set_api_key(const std::string& service, const std::string& api_key);
    void set_api_endpoint(const std::string& service, const std::string& endpoint);
    void enable_testnet(bool testnet = false);

    /**
     * Get wallet statistics
     */
    struct WalletStats {
        int total_keys;
        int compressed_keys;
        int uncompressed_keys;
        int funded_addresses;
        uint64_t total_balance;
        std::string creation_time;
        std::string last_transaction;
    };
    WalletStats get_wallet_stats(const std::vector<PrivateKeyInfo>& keys);

private:
    // Berkeley DB parsing
    struct BDBHeader {
        uint32_t magic;
        uint32_t version;
        uint32_t page_size;
        uint32_t flags;
    };

    struct BDBPage {
        uint32_t page_number;
        uint32_t page_type;
        uint32_t prev_page;
        uint32_t next_page;
        std::vector<uint8_t> data;
    };

    // Wallet data structures
    struct MasterKey {
        std::vector<uint8_t> encrypted_key;
        std::vector<uint8_t> salt;
        uint32_t derive_iterations;
        uint32_t derive_method;
        std::vector<uint8_t> other_params;
    };

    struct CryptedKey {
        std::vector<uint8_t> public_key;
        std::vector<uint8_t> encrypted_private_key;
    };

    // Internal data
    std::vector<uint8_t> wallet_data_;
    std::map<std::string, MasterKey> master_keys_;
    std::map<std::string, CryptedKey> crypted_keys_;
    std::map<std::string, std::string> key_labels_;
    std::map<std::string, std::string> api_keys_;
    std::map<std::string, std::string> api_endpoints_;
    bool testnet_mode_;
    bool loaded_;

    // Berkeley DB parsing methods
    bool parse_bdb_file();
    bool parse_bdb_page(const std::vector<uint8_t>& page_data, size_t offset);
    bool extract_key_value_pairs(const std::vector<uint8_t>& data);

    // Wallet decryption methods
    bool decrypt_master_key(const std::string& password, const MasterKey& master_key, 
                           std::vector<uint8_t>& decrypted_key);
    bool decrypt_private_key(const std::vector<uint8_t>& master_key, 
                            const CryptedKey& crypted_key,
                            std::vector<uint8_t>& private_key);

    // Key format conversion methods
    std::string private_key_to_wif(const std::vector<uint8_t>& private_key, bool compressed = true);
    std::string public_key_to_address(const std::vector<uint8_t>& public_key, bool compressed = true);
    std::vector<uint8_t> private_key_to_public_key(const std::vector<uint8_t>& private_key);

    // Blockchain API methods
    bool query_address_balance(const std::string& address, uint64_t& balance, int& tx_count);
    bool query_blockstream_api(const std::string& address, uint64_t& balance, int& tx_count);
    bool query_blockchair_api(const std::string& address, uint64_t& balance, int& tx_count);
    bool query_blockcypher_api(const std::string& address, uint64_t& balance, int& tx_count);

    // Utility methods
    std::vector<uint8_t> derive_key(const std::string& password, const std::vector<uint8_t>& salt, 
                                   uint32_t iterations);
    bool verify_key_pair(const std::vector<uint8_t>& private_key, 
                        const std::vector<uint8_t>& public_key);
    std::string format_balance(uint64_t satoshis);
    std::string get_current_timestamp();

    // Error handling
    void set_error_context(const std::string& context, const std::string& details);
};
