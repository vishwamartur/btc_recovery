# Bitcoin Wallet Password Recovery System

A high-performance, multi-threaded Bitcoin wallet password recovery system with GPU acceleration support.

## Features

- **Multi-threaded C++ Core**: Optimized brute-force password recovery engine
- **GPU Acceleration**: CUDA and OpenCL support for discrete and integrated graphics
- **Lightweight wallet.dat Recovery**: No blockchain download required (saves 400+ GB)
- **Multiple Wallet Formats**: Support for wallet.dat, Electrum, and other common formats
- **Blockchain API Integration**: Real-time balance checking without full Bitcoin node
- **Private Key Extraction**: Export keys in WIF, hex, JSON, CSV, and Electrum formats
- **Scalable Architecture**: Designed for Linux clusters and AWS EC2 deployment
- **Advanced Password Generation**: Configurable permutation algorithms and dictionary support
- **Progress Tracking**: Real-time progress monitoring and result logging
- **Thread-Safe Operations**: Optimized for multi-core processing

## Supported Wallet Formats

- **Bitcoin Core wallet.dat files** (with blockchain API balance checking)
- Electrum wallet files
- MultiBit wallet files
- Armory wallet files
- BIP38 encrypted private keys

## Quick Start - wallet.dat Recovery

### Prerequisites
```bash
# Install dependencies
sudo apt-get install -y build-essential cmake libssl-dev libcurl4-openssl-dev libjsoncpp-dev

# Clone and build
git clone https://github.com/vishwamartur/btc_recovery.git
cd btc_recovery
./scripts/build.sh
```

### Basic Recovery
```bash
# Analyze wallet structure
./build/btc-recovery --analyze-wallet /path/to/wallet.dat

# Quick recovery with common passwords
./build/btc-recovery --config config/wallet_dat_recovery.yaml --preset quick --wallet /path/to/wallet.dat

# Comprehensive recovery (dictionary + brute force)
./build/btc-recovery --config config/wallet_dat_recovery.yaml --preset comprehensive --wallet /path/to/wallet.dat
```

### Key Features for wallet.dat Recovery
- ✅ **No blockchain download** - Uses blockchain APIs for balance checking
- ✅ **Private key extraction** - Exports keys in multiple formats
- ✅ **Real-time balance checking** - Shows which addresses have funds
- ✅ **Multiple export formats** - Text, JSON, CSV, Electrum-compatible
- ✅ **Secure processing** - Works offline, only connects for balance checks

## Project Structure

```
btc-recovery/
├── src/                    # C++ source files
│   ├── core/              # Core recovery engine
│   ├── wallets/           # Wallet format handlers
│   ├── gpu/               # GPU acceleration modules
│   └── utils/             # Utility functions
├── include/               # Header files
├── config/                # Configuration files
├── scripts/               # Build and deployment scripts
├── docs/                  # Documentation
├── tests/                 # Unit tests
└── examples/              # Example configurations
```

## Quick Start

### Prerequisites

- C++17 compatible compiler (GCC 8+ or Clang 7+)
- CMake 3.15+
- OpenSSL development libraries
- CUDA Toolkit (optional, for GPU acceleration)
- OpenCL development libraries (optional)

### Building

```bash
# Clone and navigate to project
cd btc-recovery

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the project
make -j$(nproc)
```

### Basic Usage

```bash
# Basic brute-force recovery
./btc-recovery --wallet wallet.dat --charset lowercase --min-length 6 --max-length 12

# Dictionary attack
./btc-recovery --wallet wallet.dat --dictionary passwords.txt --rules common

# GPU-accelerated recovery
./btc-recovery --wallet wallet.dat --gpu --charset mixed --threads 1024
```

## Configuration

The system uses YAML configuration files for advanced settings:

- `config/recovery.yaml`: Main recovery parameters
- `config/cluster.yaml`: Cluster deployment settings
- `config/gpu.yaml`: GPU acceleration options

## Performance Optimization

- **CPU**: Utilizes all available cores with optimized thread pools
- **Memory**: Efficient memory management with configurable buffer sizes
- **GPU**: Supports multiple GPUs with work distribution
- **Network**: Cluster coordination for distributed processing

## Security Notice

This tool is intended for legitimate wallet recovery purposes only. Users must:
- Own the wallet files they are attempting to recover
- Comply with local laws and regulations
- Use responsibly and ethically

## License

This project is for educational and legitimate recovery purposes only.

## Contributing

Please read CONTRIBUTING.md for details on our code of conduct and the process for submitting pull requests.

## Support

For support and questions, please check the documentation in the `docs/` directory.
