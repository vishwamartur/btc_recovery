#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <getopt.h>

#include "core/recovery_engine.h"
#include "core/config_manager.h"
#include "utils/logger.h"

void print_usage(const char* program_name) {
    std::cout << "Bitcoin Wallet Password Recovery System\n\n";
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
    std::cout << "Required Options:\n";
    std::cout << "  -w, --wallet FILE         Path to wallet file\n\n";
    std::cout << "Recovery Options:\n";
    std::cout << "  -c, --charset TYPE        Character set (lowercase, uppercase, digits, mixed, custom)\n";
    std::cout << "  -d, --dictionary FILE     Dictionary file for dictionary attack\n";
    std::cout << "  -r, --rules FILE          Password rules file\n";
    std::cout << "  -m, --min-length N        Minimum password length (default: 1)\n";
    std::cout << "  -M, --max-length N        Maximum password length (default: 12)\n";
    std::cout << "  -p, --prefix STRING       Password prefix\n";
    std::cout << "  -s, --suffix STRING       Password suffix\n\n";
    std::cout << "Performance Options:\n";
    std::cout << "  -t, --threads N           Number of CPU threads (default: auto)\n";
    std::cout << "  -g, --gpu                 Enable GPU acceleration\n";
    std::cout << "  -G, --gpu-threads N       Number of GPU threads (default: 1024)\n";
    std::cout << "  -b, --batch-size N        Batch size for processing (default: 10000)\n\n";
    std::cout << "Output Options:\n";
    std::cout << "  -o, --output FILE         Output file for results\n";
    std::cout << "  -l, --log-level LEVEL     Log level (debug, info, warn, error)\n";
    std::cout << "  -q, --quiet               Suppress progress output\n\n";
    std::cout << "Configuration:\n";
    std::cout << "  -C, --config FILE         Configuration file\n";
    std::cout << "  -h, --help                Show this help message\n";
    std::cout << "  -v, --version             Show version information\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " -w wallet.dat -c lowercase -m 6 -M 10\n";
    std::cout << "  " << program_name << " -w wallet.dat -d passwords.txt -r common.rules\n";
    std::cout << "  " << program_name << " -w wallet.dat -c mixed -g -t 8 -G 2048\n";
}

void print_version() {
    std::cout << "Bitcoin Wallet Password Recovery System v1.0.0\n";
    std::cout << "Built with C++17 support\n";
#ifdef ENABLE_CUDA
    std::cout << "CUDA support: Enabled\n";
#else
    std::cout << "CUDA support: Disabled\n";
#endif
#ifdef ENABLE_OPENCL
    std::cout << "OpenCL support: Enabled\n";
#else
    std::cout << "OpenCL support: Disabled\n";
#endif
}

int main(int argc, char* argv[]) {
    // Command line options
    struct option long_options[] = {
        {"wallet", required_argument, 0, 'w'},
        {"charset", required_argument, 0, 'c'},
        {"dictionary", required_argument, 0, 'd'},
        {"rules", required_argument, 0, 'r'},
        {"min-length", required_argument, 0, 'm'},
        {"max-length", required_argument, 0, 'M'},
        {"prefix", required_argument, 0, 'p'},
        {"suffix", required_argument, 0, 's'},
        {"threads", required_argument, 0, 't'},
        {"gpu", no_argument, 0, 'g'},
        {"gpu-threads", required_argument, 0, 'G'},
        {"batch-size", required_argument, 0, 'b'},
        {"output", required_argument, 0, 'o'},
        {"log-level", required_argument, 0, 'l'},
        {"quiet", no_argument, 0, 'q'},
        {"config", required_argument, 0, 'C'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    // Initialize configuration
    auto config = std::make_shared<ConfigManager>();
    
    std::string wallet_file;
    std::string charset = "mixed";
    std::string dictionary_file;
    std::string rules_file;
    int min_length = 1;
    int max_length = 12;
    std::string prefix;
    std::string suffix;
    int threads = 0; // 0 = auto-detect
    bool use_gpu = false;
    int gpu_threads = 1024;
    int batch_size = 10000;
    std::string output_file;
    std::string log_level = "info";
    bool quiet = false;
    std::string config_file;

    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "w:c:d:r:m:M:p:s:t:gG:b:o:l:qC:hv", 
                           long_options, &option_index)) != -1) {
        switch (c) {
            case 'w': wallet_file = optarg; break;
            case 'c': charset = optarg; break;
            case 'd': dictionary_file = optarg; break;
            case 'r': rules_file = optarg; break;
            case 'm': min_length = std::stoi(optarg); break;
            case 'M': max_length = std::stoi(optarg); break;
            case 'p': prefix = optarg; break;
            case 's': suffix = optarg; break;
            case 't': threads = std::stoi(optarg); break;
            case 'g': use_gpu = true; break;
            case 'G': gpu_threads = std::stoi(optarg); break;
            case 'b': batch_size = std::stoi(optarg); break;
            case 'o': output_file = optarg; break;
            case 'l': log_level = optarg; break;
            case 'q': quiet = true; break;
            case 'C': config_file = optarg; break;
            case 'h': print_usage(argv[0]); return 0;
            case 'v': print_version(); return 0;
            case '?': print_usage(argv[0]); return 1;
            default: break;
        }
    }

    // Validate required arguments
    if (wallet_file.empty()) {
        std::cerr << "Error: Wallet file is required\n";
        print_usage(argv[0]);
        return 1;
    }

    // Initialize logger
    Logger::initialize(log_level, !quiet);
    Logger::info("Bitcoin Wallet Password Recovery System v1.0.0");
    Logger::info("Starting recovery process...");

    try {
        // Load configuration file if specified
        if (!config_file.empty()) {
            config->load_config(config_file);
        }

        // Set configuration parameters
        config->set_wallet_file(wallet_file);
        config->set_charset(charset);
        config->set_dictionary_file(dictionary_file);
        config->set_rules_file(rules_file);
        config->set_min_length(min_length);
        config->set_max_length(max_length);
        config->set_prefix(prefix);
        config->set_suffix(suffix);
        config->set_threads(threads);
        config->set_use_gpu(use_gpu);
        config->set_gpu_threads(gpu_threads);
        config->set_batch_size(batch_size);
        config->set_output_file(output_file);

        // Create and run recovery engine
        RecoveryEngine engine(config);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        bool success = engine.run();
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        
        if (success) {
            Logger::info("Password recovery completed successfully!");
            Logger::info("Total time: " + std::to_string(duration.count()) + " seconds");
            return 0;
        } else {
            Logger::info("Password recovery completed without finding the password");
            Logger::info("Total time: " + std::to_string(duration.count()) + " seconds");
            return 2;
        }
        
    } catch (const std::exception& e) {
        Logger::error("Error: " + std::string(e.what()));
        return 1;
    }
}
