#include "wallets/bitcoin_core_wallet.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <curl/curl.h>
#include <json/json.h>

BitcoinCoreWallet::BitcoinCoreWallet(const std::string& wallet_file) 
    : WalletBase(wallet_file), testnet_mode_(false), loaded_(false) {
    
    // Initialize default API endpoints
    api_endpoints_["blockstream"] = "https://blockstream.info/api";
    api_endpoints_["blockchair"] = "https://api.blockchair.com/bitcoin";
    api_endpoints_["blockcypher"] = "https://api.blockcypher.com/v1/btc/main";
    
    // Testnet endpoints
    api_endpoints_["blockstream_testnet"] = "https://blockstream.info/testnet/api";
    api_endpoints_["blockcypher_testnet"] = "https://api.blockcypher.com/v1/btc/test3";
}

bool BitcoinCoreWallet::load() {
    if (loaded_) {
        return true;
    }

    Logger::info("Loading Bitcoin Core wallet: " + wallet_file_);

    if (!verify_file_access(wallet_file_)) {
        set_error("Cannot access wallet file: " + wallet_file_);
        return false;
    }

    // Read wallet file
    wallet_data_ = read_file(wallet_file_);
    if (wallet_data_.empty()) {
        set_error("Failed to read wallet file or file is empty");
        return false;
    }

    // Parse Berkeley DB format
    if (!parse_bdb_file()) {
        set_error("Failed to parse wallet.dat file - invalid format");
        return false;
    }

    loaded_ = true;
    Logger::info("Successfully loaded wallet with " + std::to_string(crypted_keys_.size()) + " encrypted keys");
    
    return true;
}

bool BitcoinCoreWallet::test_password(const std::string& password) {
    if (!loaded_ && !load()) {
        return false;
    }

    if (master_keys_.empty()) {
        set_error("No master keys found in wallet");
        return false;
    }

    // Try to decrypt the first master key
    for (const auto& mk_pair : master_keys_) {
        const MasterKey& master_key = mk_pair.second;
        std::vector<uint8_t> decrypted_key;
        
        if (decrypt_master_key(password, master_key, decrypted_key)) {
            Logger::debug("Password verification successful");
            return true;
        }
    }

    return false;
}

WalletRecoveryResult BitcoinCoreWallet::recover_wallet(const std::string& password) {
    WalletRecoveryResult result;
    result.success = false;
    result.password = password;
    result.total_balance_satoshis = 0;
    result.total_addresses = 0;
    result.funded_addresses = 0;
    result.recovery_timestamp = get_current_timestamp();

    Logger::info("Starting wallet recovery with password verification...");

    if (!test_password(password)) {
        set_error("Invalid password provided");
        return result;
    }

    Logger::info("Password verified successfully, extracting private keys...");

    // Extract private keys
    result.private_keys = extract_private_keys(password);
    if (result.private_keys.empty()) {
        set_error("No private keys could be extracted");
        return result;
    }

    result.total_addresses = result.private_keys.size();
    Logger::info("Extracted " + std::to_string(result.total_addresses) + " private keys");

    // Check balances using blockchain APIs
    Logger::info("Checking balances for all addresses...");
    if (check_balances(result.private_keys)) {
        // Calculate totals
        for (const auto& key_info : result.private_keys) {
            result.total_balance_satoshis += key_info.balance_satoshis;
            if (key_info.has_balance) {
                result.funded_addresses++;
            }
        }
        
        Logger::info("Balance check completed:");
        Logger::info("  Total addresses: " + std::to_string(result.total_addresses));
        Logger::info("  Funded addresses: " + std::to_string(result.funded_addresses));
        Logger::info("  Total balance: " + format_balance(result.total_balance_satoshis) + " BTC");
    } else {
        Logger::warn("Balance check failed - continuing without balance information");
    }

    result.success = true;
    return result;
}

std::vector<PrivateKeyInfo> BitcoinCoreWallet::extract_private_keys(const std::string& password) {
    std::vector<PrivateKeyInfo> private_keys;

    if (!loaded_ && !load()) {
        return private_keys;
    }

    // Decrypt master key first
    std::vector<uint8_t> master_key;
    bool master_key_found = false;

    for (const auto& mk_pair : master_keys_) {
        if (decrypt_master_key(password, mk_pair.second, master_key)) {
            master_key_found = true;
            break;
        }
    }

    if (!master_key_found) {
        set_error("Failed to decrypt master key with provided password");
        return private_keys;
    }

    Logger::info("Master key decrypted, processing " + std::to_string(crypted_keys_.size()) + " encrypted keys...");

    // Decrypt all private keys
    for (const auto& ck_pair : crypted_keys_) {
        const CryptedKey& crypted_key = ck_pair.second;
        std::vector<uint8_t> private_key_bytes;

        if (decrypt_private_key(master_key, crypted_key, private_key_bytes)) {
            PrivateKeyInfo key_info;
            
            // Convert private key to different formats
            key_info.private_key_hex = "";
            for (uint8_t byte : private_key_bytes) {
                std::stringstream ss;
                ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
                key_info.private_key_hex += ss.str();
            }

            // Generate public key and address
            std::vector<uint8_t> public_key = private_key_to_public_key(private_key_bytes);
            key_info.public_key_hex = "";
            for (uint8_t byte : public_key) {
                std::stringstream ss;
                ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
                key_info.public_key_hex += ss.str();
            }

            // Try both compressed and uncompressed formats
            std::string compressed_addr = public_key_to_address(public_key, true);
            std::string uncompressed_addr = public_key_to_address(public_key, false);

            // Use compressed format by default (more common in modern wallets)
            key_info.address = compressed_addr;
            key_info.compressed = true;
            key_info.private_key_wif = private_key_to_wif(private_key_bytes, true);

            // Check if we have a label for this key
            auto label_it = key_labels_.find(key_info.address);
            if (label_it != key_labels_.end()) {
                key_info.label = label_it->second;
            }

            // Initialize balance fields
            key_info.balance_satoshis = 0;
            key_info.transaction_count = 0;
            key_info.has_balance = false;

            private_keys.push_back(key_info);

            // Also add uncompressed version if different
            if (compressed_addr != uncompressed_addr) {
                PrivateKeyInfo uncompressed_key_info = key_info;
                uncompressed_key_info.address = uncompressed_addr;
                uncompressed_key_info.compressed = false;
                uncompressed_key_info.private_key_wif = private_key_to_wif(private_key_bytes, false);
                
                auto uncompressed_label_it = key_labels_.find(uncompressed_addr);
                if (uncompressed_label_it != key_labels_.end()) {
                    uncompressed_key_info.label = uncompressed_label_it->second;
                }
                
                private_keys.push_back(uncompressed_key_info);
            }
        }
    }

    Logger::info("Successfully extracted " + std::to_string(private_keys.size()) + " private keys");
    return private_keys;
}

bool BitcoinCoreWallet::check_balances(std::vector<PrivateKeyInfo>& private_keys) {
    if (private_keys.empty()) {
        return false;
    }

    Logger::info("Checking balances for " + std::to_string(private_keys.size()) + " addresses...");

    int successful_queries = 0;
    int failed_queries = 0;

    for (auto& key_info : private_keys) {
        uint64_t balance = 0;
        int tx_count = 0;

        if (query_address_balance(key_info.address, balance, tx_count)) {
            key_info.balance_satoshis = balance;
            key_info.transaction_count = tx_count;
            key_info.has_balance = (balance > 0);
            successful_queries++;

            if (balance > 0) {
                Logger::info("Found balance: " + key_info.address + " = " + 
                           format_balance(balance) + " BTC (" + std::to_string(tx_count) + " txs)");
            }
        } else {
            failed_queries++;
            Logger::debug("Failed to query balance for: " + key_info.address);
        }

        // Add small delay to avoid rate limiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Logger::info("Balance check completed: " + std::to_string(successful_queries) + 
                " successful, " + std::to_string(failed_queries) + " failed");

    return successful_queries > 0;
}

bool BitcoinCoreWallet::parse_bdb_file() {
    if (wallet_data_.size() < sizeof(BDBHeader)) {
        set_error("Wallet file too small to contain valid Berkeley DB header");
        return false;
    }

    // Check Berkeley DB magic number
    uint32_t magic = *reinterpret_cast<const uint32_t*>(wallet_data_.data());
    if (magic != 0x00061561 && magic != 0x61150600) { // Little and big endian
        set_error("Invalid Berkeley DB magic number");
        return false;
    }

    Logger::debug("Valid Berkeley DB format detected");

    // Parse the database pages to extract key-value pairs
    size_t offset = 0;
    while (offset < wallet_data_.size()) {
        if (!parse_bdb_page(wallet_data_, offset)) {
            break; // Continue parsing even if some pages fail
        }
    }

    Logger::info("Parsed wallet.dat: found " + std::to_string(master_keys_.size()) + 
                " master keys and " + std::to_string(crypted_keys_.size()) + " encrypted keys");

    return !master_keys_.empty() && !crypted_keys_.empty();
}

WalletMetadata BitcoinCoreWallet::get_metadata() const {
    WalletMetadata metadata;
    metadata.format = WalletFormat::BITCOIN_CORE;
    metadata.encryption = EncryptionType::AES_256_CBC;
    metadata.version = "Bitcoin Core";
    metadata.iterations = 25000; // Default for Bitcoin Core
    metadata.key_length = 32;
    metadata.iv_length = 16;
    
    if (!master_keys_.empty()) {
        const auto& first_master_key = master_keys_.begin()->second;
        metadata.iterations = first_master_key.derive_iterations;
        metadata.salt = first_master_key.salt;
    }
    
    return metadata;
}

bool BitcoinCoreWallet::is_valid() const {
    return loaded_ && !master_keys_.empty() && !crypted_keys_.empty();
}

WalletFormat BitcoinCoreWallet::get_format() const {
    return WalletFormat::BITCOIN_CORE;
}

EncryptionType BitcoinCoreWallet::get_encryption_type() const {
    return EncryptionType::AES_256_CBC;
}

uint64_t BitcoinCoreWallet::get_estimated_test_time() const {
    // Bitcoin Core uses relatively slow key derivation
    return 50000; // 50ms per password test
}

bool BitcoinCoreWallet::decrypt_master_key(const std::string& password, const MasterKey& master_key,
                                          std::vector<uint8_t>& decrypted_key) {
    // Derive key from password using PBKDF2
    std::vector<uint8_t> derived_key = derive_key(password, master_key.salt, master_key.derive_iterations);

    if (derived_key.size() != 32) {
        return false;
    }

    // Decrypt the master key using AES-256-CBC
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return false;
    }

    bool success = false;
    decrypted_key.resize(master_key.encrypted_key.size());
    int len = 0, final_len = 0;

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, derived_key.data(),
                          master_key.encrypted_key.data()) == 1) {
        if (EVP_DecryptUpdate(ctx, decrypted_key.data(), &len,
                             master_key.encrypted_key.data() + 16,
                             master_key.encrypted_key.size() - 16) == 1) {
            if (EVP_DecryptFinal_ex(ctx, decrypted_key.data() + len, &final_len) == 1) {
                decrypted_key.resize(len + final_len);
                success = true;
            }
        }
    }

    EVP_CIPHER_CTX_free(ctx);
    return success;
}

bool BitcoinCoreWallet::decrypt_private_key(const std::vector<uint8_t>& master_key,
                                           const CryptedKey& crypted_key,
                                           std::vector<uint8_t>& private_key) {
    if (master_key.size() != 32 || crypted_key.encrypted_private_key.size() < 16) {
        return false;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return false;
    }

    bool success = false;
    private_key.resize(crypted_key.encrypted_private_key.size());
    int len = 0, final_len = 0;

    // Use first 16 bytes as IV
    const uint8_t* iv = crypted_key.encrypted_private_key.data();
    const uint8_t* encrypted_data = crypted_key.encrypted_private_key.data() + 16;
    int encrypted_len = crypted_key.encrypted_private_key.size() - 16;

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, master_key.data(), iv) == 1) {
        if (EVP_DecryptUpdate(ctx, private_key.data(), &len, encrypted_data, encrypted_len) == 1) {
            if (EVP_DecryptFinal_ex(ctx, private_key.data() + len, &final_len) == 1) {
                private_key.resize(len + final_len);
                success = true;
            }
        }
    }

    EVP_CIPHER_CTX_free(ctx);
    return success;
}

std::vector<uint8_t> BitcoinCoreWallet::derive_key(const std::string& password,
                                                  const std::vector<uint8_t>& salt,
                                                  uint32_t iterations) {
    std::vector<uint8_t> derived_key(32);

    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                         salt.data(), salt.size(),
                         iterations, EVP_sha512(),
                         32, derived_key.data()) != 1) {
        derived_key.clear();
    }

    return derived_key;
}

std::string BitcoinCoreWallet::private_key_to_wif(const std::vector<uint8_t>& private_key, bool compressed) {
    if (private_key.size() != 32) {
        return "";
    }

    std::vector<uint8_t> wif_data;

    // Add version byte (0x80 for mainnet, 0xEF for testnet)
    wif_data.push_back(testnet_mode_ ? 0xEF : 0x80);

    // Add private key
    wif_data.insert(wif_data.end(), private_key.begin(), private_key.end());

    // Add compression flag if compressed
    if (compressed) {
        wif_data.push_back(0x01);
    }

    // Calculate checksum (double SHA256)
    uint8_t hash1[SHA256_DIGEST_LENGTH];
    uint8_t hash2[SHA256_DIGEST_LENGTH];
    SHA256(wif_data.data(), wif_data.size(), hash1);
    SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);

    // Add first 4 bytes of checksum
    wif_data.insert(wif_data.end(), hash2, hash2 + 4);

    // Base58 encode (simplified implementation)
    return base58_encode(wif_data);
}

std::string BitcoinCoreWallet::public_key_to_address(const std::vector<uint8_t>& public_key, bool compressed) {
    if (public_key.empty()) {
        return "";
    }

    std::vector<uint8_t> pub_key = public_key;

    // Compress public key if needed
    if (compressed && pub_key.size() == 65) {
        pub_key.resize(33);
        pub_key[0] = (public_key[64] % 2) ? 0x03 : 0x02;
    }

    // Hash160 (SHA256 then RIPEMD160)
    uint8_t sha256_hash[SHA256_DIGEST_LENGTH];
    SHA256(pub_key.data(), pub_key.size(), sha256_hash);

    uint8_t ripemd160_hash[RIPEMD160_DIGEST_LENGTH];
    RIPEMD160(sha256_hash, SHA256_DIGEST_LENGTH, ripemd160_hash);

    // Add version byte and checksum
    std::vector<uint8_t> address_data;
    address_data.push_back(testnet_mode_ ? 0x6F : 0x00); // Version byte
    address_data.insert(address_data.end(), ripemd160_hash, ripemd160_hash + RIPEMD160_DIGEST_LENGTH);

    // Calculate checksum
    uint8_t hash1[SHA256_DIGEST_LENGTH];
    uint8_t hash2[SHA256_DIGEST_LENGTH];
    SHA256(address_data.data(), address_data.size(), hash1);
    SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);

    // Add checksum
    address_data.insert(address_data.end(), hash2, hash2 + 4);

    return base58_encode(address_data);
}

bool BitcoinCoreWallet::query_address_balance(const std::string& address, uint64_t& balance, int& tx_count) {
    balance = 0;
    tx_count = 0;

    // Try different APIs in order of preference
    if (query_blockstream_api(address, balance, tx_count)) {
        return true;
    }

    if (query_blockchair_api(address, balance, tx_count)) {
        return true;
    }

    if (query_blockcypher_api(address, balance, tx_count)) {
        return true;
    }

    return false;
}

// Callback function for CURL to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t total_size = size * nmemb;
    response->append((char*)contents, total_size);
    return total_size;
}

bool BitcoinCoreWallet::query_blockstream_api(const std::string& address, uint64_t& balance, int& tx_count) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    std::string endpoint = testnet_mode_ ? api_endpoints_["blockstream_testnet"] : api_endpoints_["blockstream"];
    std::string url = endpoint + "/address/" + address;
    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "btc-recovery/1.0");

    CURLcode res = curl_easy_perform(curl);
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || response_code != 200) {
        return false;
    }

    // Parse JSON response
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) {
        return false;
    }

    if (root.isMember("chain_stats")) {
        const Json::Value& stats = root["chain_stats"];
        if (stats.isMember("funded_txo_sum")) {
            balance = stats["funded_txo_sum"].asUInt64();
        }
        if (stats.isMember("tx_count")) {
            tx_count = stats["tx_count"].asInt();
        }
        return true;
    }

    return false;
}

bool BitcoinCoreWallet::query_blockchair_api(const std::string& address, uint64_t& balance, int& tx_count) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    std::string url = api_endpoints_["blockchair"] + "/dashboards/address/" + address;
    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "btc-recovery/1.0");

    CURLcode res = curl_easy_perform(curl);
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || response_code != 200) {
        return false;
    }

    // Parse JSON response
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) {
        return false;
    }

    if (root.isMember("data") && root["data"].isMember(address)) {
        const Json::Value& addr_data = root["data"][address]["address"];
        if (addr_data.isMember("balance")) {
            balance = addr_data["balance"].asUInt64();
        }
        if (addr_data.isMember("transaction_count")) {
            tx_count = addr_data["transaction_count"].asInt();
        }
        return true;
    }

    return false;
}

bool BitcoinCoreWallet::query_blockcypher_api(const std::string& address, uint64_t& balance, int& tx_count) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    std::string endpoint = testnet_mode_ ? api_endpoints_["blockcypher_testnet"] : api_endpoints_["blockcypher"];
    std::string url = endpoint + "/addrs/" + address + "/balance";

    // Add API key if available
    if (api_keys_.find("blockcypher") != api_keys_.end()) {
        url += "?token=" + api_keys_["blockcypher"];
    }

    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "btc-recovery/1.0");

    CURLcode res = curl_easy_perform(curl);
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || response_code != 200) {
        return false;
    }

    // Parse JSON response
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) {
        return false;
    }

    if (root.isMember("balance")) {
        balance = root["balance"].asUInt64();
    }
    if (root.isMember("n_tx")) {
        tx_count = root["n_tx"].asInt();
    }

    return true;
}

bool BitcoinCoreWallet::export_to_text(const std::vector<PrivateKeyInfo>& keys, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        set_error("Cannot create output file: " + filename);
        return false;
    }

    file << "Bitcoin Wallet Recovery Results\n";
    file << "Generated: " << get_current_timestamp() << "\n";
    file << "Total Addresses: " << keys.size() << "\n\n";

    uint64_t total_balance = 0;
    int funded_count = 0;

    for (const auto& key : keys) {
        file << "Address: " << key.address << "\n";
        file << "Private Key (WIF): " << key.private_key_wif << "\n";
        file << "Private Key (Hex): " << key.private_key_hex << "\n";
        file << "Public Key: " << key.public_key_hex << "\n";
        file << "Compressed: " << (key.compressed ? "Yes" : "No") << "\n";

        if (!key.label.empty()) {
            file << "Label: " << key.label << "\n";
        }

        file << "Balance: " << format_balance(key.balance_satoshis) << " BTC\n";
        file << "Transactions: " << key.transaction_count << "\n";
        file << "Has Funds: " << (key.has_balance ? "Yes" : "No") << "\n";
        file << "\n";

        total_balance += key.balance_satoshis;
        if (key.has_balance) funded_count++;
    }

    file << "Summary:\n";
    file << "Total Balance: " << format_balance(total_balance) << " BTC\n";
    file << "Funded Addresses: " << funded_count << "/" << keys.size() << "\n";

    file.close();
    Logger::info("Exported recovery results to: " + filename);
    return true;
}

bool BitcoinCoreWallet::export_to_json(const std::vector<PrivateKeyInfo>& keys, const std::string& filename) {
    Json::Value root;
    Json::Value addresses(Json::arrayValue);

    root["recovery_timestamp"] = get_current_timestamp();
    root["total_addresses"] = (int)keys.size();

    uint64_t total_balance = 0;
    int funded_count = 0;

    for (const auto& key : keys) {
        Json::Value addr;
        addr["address"] = key.address;
        addr["private_key_wif"] = key.private_key_wif;
        addr["private_key_hex"] = key.private_key_hex;
        addr["public_key_hex"] = key.public_key_hex;
        addr["compressed"] = key.compressed;
        addr["label"] = key.label;
        addr["balance_satoshis"] = (Json::UInt64)key.balance_satoshis;
        addr["balance_btc"] = format_balance(key.balance_satoshis);
        addr["transaction_count"] = key.transaction_count;
        addr["has_balance"] = key.has_balance;

        addresses.append(addr);
        total_balance += key.balance_satoshis;
        if (key.has_balance) funded_count++;
    }

    root["addresses"] = addresses;
    root["total_balance_satoshis"] = (Json::UInt64)total_balance;
    root["total_balance_btc"] = format_balance(total_balance);
    root["funded_addresses"] = funded_count;

    std::ofstream file(filename);
    if (!file.is_open()) {
        set_error("Cannot create output file: " + filename);
        return false;
    }

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(root, &file);

    file.close();
    Logger::info("Exported recovery results to JSON: " + filename);
    return true;
}

bool BitcoinCoreWallet::export_to_csv(const std::vector<PrivateKeyInfo>& keys, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        set_error("Cannot create output file: " + filename);
        return false;
    }

    // CSV header
    file << "Address,Private_Key_WIF,Private_Key_Hex,Public_Key_Hex,Compressed,Label,Balance_BTC,Balance_Satoshis,Transaction_Count,Has_Balance\n";

    for (const auto& key : keys) {
        file << key.address << ","
             << key.private_key_wif << ","
             << key.private_key_hex << ","
             << key.public_key_hex << ","
             << (key.compressed ? "true" : "false") << ","
             << "\"" << key.label << "\","
             << format_balance(key.balance_satoshis) << ","
             << key.balance_satoshis << ","
             << key.transaction_count << ","
             << (key.has_balance ? "true" : "false") << "\n";
    }

    file.close();
    Logger::info("Exported recovery results to CSV: " + filename);
    return true;
}

bool BitcoinCoreWallet::export_to_electrum(const std::vector<PrivateKeyInfo>& keys, const std::string& filename) {
    Json::Value root;
    Json::Value keystore;
    Json::Value keypairs;

    // Electrum wallet format
    keystore["type"] = "imported";

    for (const auto& key : keys) {
        if (key.has_balance || key.transaction_count > 0) {
            keypairs[key.address] = key.private_key_wif;
        }
    }

    keystore["keypairs"] = keypairs;
    root["keystore"] = keystore;
    root["wallet_type"] = "standard";
    root["use_encryption"] = false;

    std::ofstream file(filename);
    if (!file.is_open()) {
        set_error("Cannot create output file: " + filename);
        return false;
    }

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(root, &file);

    file.close();
    Logger::info("Exported Electrum-compatible wallet: " + filename);
    return true;
}

void BitcoinCoreWallet::set_api_key(const std::string& service, const std::string& api_key) {
    api_keys_[service] = api_key;
    Logger::info("API key set for service: " + service);
}

void BitcoinCoreWallet::set_api_endpoint(const std::string& service, const std::string& endpoint) {
    api_endpoints_[service] = endpoint;
    Logger::info("API endpoint set for " + service + ": " + endpoint);
}

void BitcoinCoreWallet::enable_testnet(bool testnet) {
    testnet_mode_ = testnet;
    Logger::info("Testnet mode: " + std::string(testnet ? "enabled" : "disabled"));
}

BitcoinCoreWallet::WalletStats BitcoinCoreWallet::get_wallet_stats(const std::vector<PrivateKeyInfo>& keys) {
    WalletStats stats;
    stats.total_keys = keys.size();
    stats.compressed_keys = 0;
    stats.uncompressed_keys = 0;
    stats.funded_addresses = 0;
    stats.total_balance = 0;
    stats.creation_time = "Unknown";
    stats.last_transaction = "Unknown";

    for (const auto& key : keys) {
        if (key.compressed) {
            stats.compressed_keys++;
        } else {
            stats.uncompressed_keys++;
        }

        if (key.has_balance) {
            stats.funded_addresses++;
        }

        stats.total_balance += key.balance_satoshis;
    }

    return stats;
}

std::string BitcoinCoreWallet::format_balance(uint64_t satoshis) {
    double btc = static_cast<double>(satoshis) / 100000000.0;
    std::stringstream ss;
    ss << std::fixed << std::setprecision(8) << btc;
    return ss.str();
}

std::string BitcoinCoreWallet::get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Base58 encoding implementation
static const char* base58_alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

std::string BitcoinCoreWallet::base58_encode(const std::vector<uint8_t>& data) {
    // Count leading zeros
    int leading_zeros = 0;
    for (uint8_t byte : data) {
        if (byte == 0) {
            leading_zeros++;
        } else {
            break;
        }
    }

    // Convert to base58
    std::vector<uint8_t> digits;
    for (size_t i = leading_zeros; i < data.size(); i++) {
        int carry = data[i];
        for (size_t j = 0; j < digits.size(); j++) {
            carry += digits[j] * 256;
            digits[j] = carry % 58;
            carry /= 58;
        }
        while (carry > 0) {
            digits.push_back(carry % 58);
            carry /= 58;
        }
    }

    // Build result string
    std::string result;
    for (int i = 0; i < leading_zeros; i++) {
        result += '1';
    }
    for (int i = digits.size() - 1; i >= 0; i--) {
        result += base58_alphabet[digits[i]];
    }

    return result;
}

std::vector<uint8_t> BitcoinCoreWallet::private_key_to_public_key(const std::vector<uint8_t>& private_key) {
    if (private_key.size() != 32) {
        return {};
    }

    EC_KEY* ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!ec_key) {
        return {};
    }

    BIGNUM* bn = BN_bin2bn(private_key.data(), 32, nullptr);
    if (!bn) {
        EC_KEY_free(ec_key);
        return {};
    }

    if (EC_KEY_set_private_key(ec_key, bn) != 1) {
        BN_free(bn);
        EC_KEY_free(ec_key);
        return {};
    }

    const EC_GROUP* group = EC_KEY_get0_group(ec_key);
    EC_POINT* pub_point = EC_POINT_new(group);
    if (!pub_point) {
        BN_free(bn);
        EC_KEY_free(ec_key);
        return {};
    }

    if (EC_POINT_mul(group, pub_point, bn, nullptr, nullptr, nullptr) != 1) {
        EC_POINT_free(pub_point);
        BN_free(bn);
        EC_KEY_free(ec_key);
        return {};
    }

    size_t pub_key_len = EC_POINT_point2oct(group, pub_point, POINT_CONVERSION_UNCOMPRESSED, nullptr, 0, nullptr);
    std::vector<uint8_t> public_key(pub_key_len);

    EC_POINT_point2oct(group, pub_point, POINT_CONVERSION_UNCOMPRESSED, public_key.data(), pub_key_len, nullptr);

    EC_POINT_free(pub_point);
    BN_free(bn);
    EC_KEY_free(ec_key);

    return public_key;
}

bool BitcoinCoreWallet::parse_bdb_page(const std::vector<uint8_t>& data, size_t& offset) {
    // Simplified Berkeley DB page parsing
    // This is a basic implementation - real Berkeley DB parsing is more complex

    if (offset + 1024 > data.size()) {
        return false;
    }

    // Look for key-value patterns in the page
    for (size_t i = offset; i < std::min(offset + 1024, data.size() - 32); i++) {
        // Look for master key pattern
        if (data[i] == 'm' && data[i+1] == 'k' && data[i+2] == 'e' && data[i+3] == 'y') {
            // Found master key entry
            MasterKey master_key;
            // Extract master key data (simplified)
            size_t key_start = i + 4;
            if (key_start + 64 < data.size()) {
                master_key.salt.assign(data.begin() + key_start, data.begin() + key_start + 8);
                master_key.encrypted_key.assign(data.begin() + key_start + 8, data.begin() + key_start + 56);
                master_key.derive_iterations = 25000; // Default
                master_key.derive_method = 0;
                master_keys_["default"] = master_key;
            }
        }

        // Look for crypted key pattern
        if (data[i] == 'c' && data[i+1] == 'k' && data[i+2] == 'e' && data[i+3] == 'y') {
            // Found crypted key entry
            CryptedKey crypted_key;
            size_t key_start = i + 4;
            if (key_start + 80 < data.size()) {
                crypted_key.public_key.assign(data.begin() + key_start, data.begin() + key_start + 33);
                crypted_key.encrypted_private_key.assign(data.begin() + key_start + 33, data.begin() + key_start + 81);
                crypted_keys_[std::to_string(crypted_keys_.size())] = crypted_key;
            }
        }
    }

    offset += 1024;
    return true;
}
