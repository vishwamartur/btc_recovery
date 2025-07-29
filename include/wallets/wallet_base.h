#pragma once

#include <string>
#include <vector>
#include <memory>

/**
 * Wallet encryption types
 */
enum class EncryptionType {
    UNKNOWN,
    AES_256_CBC,
    AES_256_CTR,
    SCRYPT,
    PBKDF2,
    BIP38
};

/**
 * Wallet format types
 */
enum class WalletFormat {
    UNKNOWN,
    BITCOIN_CORE,
    ELECTRUM,
    MULTIBIT,
    ARMORY,
    BIP38_KEY
};

/**
 * Wallet metadata structure
 */
struct WalletMetadata {
    WalletFormat format;
    EncryptionType encryption;
    std::string version;
    uint32_t iterations;
    std::vector<uint8_t> salt;
    std::vector<uint8_t> encrypted_data;
    std::vector<uint8_t> checksum;
    size_t key_length;
    size_t iv_length;
};

/**
 * Base class for wallet handlers
 */
class WalletBase {
public:
    explicit WalletBase(const std::string& wallet_file);
    virtual ~WalletBase() = default;

    /**
     * Load and parse the wallet file
     * @return true if successful, false otherwise
     */
    virtual bool load() = 0;

    /**
     * Test a password against the wallet
     * @param password The password to test
     * @return true if password is correct, false otherwise
     */
    virtual bool test_password(const std::string& password) = 0;

    /**
     * Get wallet metadata
     * @return WalletMetadata structure
     */
    virtual WalletMetadata get_metadata() const = 0;

    /**
     * Check if the wallet file is valid
     * @return true if valid, false otherwise
     */
    virtual bool is_valid() const = 0;

    /**
     * Get the wallet format
     * @return WalletFormat enum value
     */
    virtual WalletFormat get_format() const = 0;

    /**
     * Get the encryption type
     * @return EncryptionType enum value
     */
    virtual EncryptionType get_encryption_type() const = 0;

    /**
     * Get estimated time per password test (in microseconds)
     * @return estimated time in microseconds
     */
    virtual uint64_t get_estimated_test_time() const = 0;

    /**
     * Get the wallet file path
     * @return file path string
     */
    std::string get_file_path() const { return wallet_file_; }

    /**
     * Get error message from last operation
     * @return error message string
     */
    std::string get_last_error() const { return last_error_; }

    /**
     * Factory method to create appropriate wallet handler
     * @param wallet_file Path to wallet file
     * @return unique_ptr to WalletBase instance
     */
    static std::unique_ptr<WalletBase> create_wallet_handler(const std::string& wallet_file);

    /**
     * Detect wallet format from file
     * @param wallet_file Path to wallet file
     * @return WalletFormat enum value
     */
    static WalletFormat detect_wallet_format(const std::string& wallet_file);

protected:
    std::string wallet_file_;
    mutable std::string last_error_;
    WalletMetadata metadata_;

    /**
     * Set error message
     * @param error Error message
     */
    void set_error(const std::string& error) const { last_error_ = error; }

    /**
     * Read file contents
     * @param file_path Path to file
     * @return file contents as vector of bytes
     */
    std::vector<uint8_t> read_file(const std::string& file_path) const;

    /**
     * Verify file exists and is readable
     * @param file_path Path to file
     * @return true if file is accessible
     */
    bool verify_file_access(const std::string& file_path) const;
};

/**
 * Wallet format detection utilities
 */
class WalletDetector {
public:
    /**
     * Detect wallet format from file header
     * @param file_path Path to wallet file
     * @return WalletFormat enum value
     */
    static WalletFormat detect_format(const std::string& file_path);

    /**
     * Check if file is a Bitcoin Core wallet
     * @param data File data
     * @return true if Bitcoin Core wallet
     */
    static bool is_bitcoin_core_wallet(const std::vector<uint8_t>& data);

    /**
     * Check if file is an Electrum wallet
     * @param data File data
     * @return true if Electrum wallet
     */
    static bool is_electrum_wallet(const std::vector<uint8_t>& data);

    /**
     * Check if file is a MultiBit wallet
     * @param data File data
     * @return true if MultiBit wallet
     */
    static bool is_multibit_wallet(const std::vector<uint8_t>& data);

    /**
     * Check if string is a BIP38 encrypted key
     * @param key_string Key string to check
     * @return true if BIP38 key
     */
    static bool is_bip38_key(const std::string& key_string);

private:
    static bool check_magic_bytes(const std::vector<uint8_t>& data, 
                                 const std::vector<uint8_t>& magic);
    static bool check_json_structure(const std::vector<uint8_t>& data, 
                                   const std::vector<std::string>& required_fields);
};
