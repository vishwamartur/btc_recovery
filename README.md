# Bitcoin Wallet Password Recovery System

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![CUDA](https://img.shields.io/badge/CUDA-11.8+-green.svg)](https://developer.nvidia.com/cuda-toolkit)
[![OpenCL](https://img.shields.io/badge/OpenCL-2.0+-orange.svg)](https://www.khronos.org/opencl/)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20%7C%20macOS-lightgrey.svg)](https://github.com/vishwamartur/btc_recovery)

[![CI/CD Pipeline](https://github.com/vishwamartur/btc_recovery/actions/workflows/ci.yml/badge.svg)](https://github.com/vishwamartur/btc_recovery/actions/workflows/ci.yml)
[![Code Quality](https://github.com/vishwamartur/btc_recovery/actions/workflows/code-quality.yml/badge.svg)](https://github.com/vishwamartur/btc_recovery/actions/workflows/code-quality.yml)
[![Documentation](https://github.com/vishwamartur/btc_recovery/actions/workflows/docs.yml/badge.svg)](https://github.com/vishwamartur/btc_recovery/actions/workflows/docs.yml)
[![Release](https://github.com/vishwamartur/btc_recovery/actions/workflows/release.yml/badge.svg)](https://github.com/vishwamartur/btc_recovery/actions/workflows/release.yml)
[![Codecov](https://codecov.io/gh/vishwamartur/btc_recovery/branch/main/graph/badge.svg)](https://codecov.io/gh/vishwamartur/btc_recovery)

A high-performance, multi-threaded Bitcoin wallet password recovery system with GPU acceleration and integrated graphics support. Designed for legitimate wallet recovery purposes with **no blockchain download required** for Bitcoin Core wallet.dat files.

> ⚠️ **Legal Notice**: This software is intended for legitimate wallet recovery only. Users must own the wallets they attempt to recover and comply with all applicable laws.

## 📋 Table of Contents

- [Features](#-features)
- [Supported Wallet Formats](#-supported-wallet-formats)
- [Quick Start - wallet.dat Recovery](#-quick-start---walletdat-recovery)
- [Installation](#-installation)
- [Usage Examples](#-usage-examples)
- [Performance](#-performance)
- [Continuous Integration](#-continuous-integration)
- [Documentation](#-documentation)
- [Contributing](#-contributing)
- [License](#-license)
- [Security](#-security)

## 🚀 Features

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

## 💼 Supported Wallet Formats

| Wallet Type | Format | Blockchain Download | Balance Checking | Private Key Export |
|-------------|--------|-------------------|------------------|-------------------|
| **Bitcoin Core** | wallet.dat | ❌ Not Required | ✅ Real-time API | ✅ WIF/Hex/JSON |
| **Electrum** | .wallet | ❌ Not Required | ✅ Built-in | ✅ Multiple formats |
| **MultiBit** | .wallet | ❌ Not Required | ✅ API-based | ✅ Standard formats |
| **Armory** | .wallet | ❌ Not Required | ✅ API-based | ✅ Standard formats |
| **BIP38** | Encrypted Keys | ❌ Not Required | ✅ API-based | ✅ Decrypted WIF |

### 🎯 Bitcoin Core wallet.dat Recovery Highlights
- **No 400+ GB blockchain download required**
- **Real-time balance checking** via Blockstream, Blockchair, BlockCypher APIs
- **Complete private key extraction** with compressed/uncompressed addresses
- **Multiple export formats**: Text, JSON, CSV, Electrum-compatible
- **Offline operation** - only connects for balance verification

## 🚀 Quick Start - wallet.dat Recovery

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake git libssl-dev libcurl4-openssl-dev libjsoncpp-dev

# CentOS/RHEL
sudo yum groupinstall -y "Development Tools"
sudo yum install -y cmake3 git openssl-devel libcurl-devel jsoncpp-devel

# macOS (with Homebrew)
brew install cmake openssl curl jsoncpp
```

### Installation
```bash
# Clone the repository
git clone https://github.com/vishwamartur/btc_recovery.git
cd btc_recovery

# Build with automatic dependency detection
./scripts/build.sh

# Or build manually
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Basic Recovery
```bash
# 1. Backup your wallet.dat file first!
cp wallet.dat wallet.dat.backup

# 2. Analyze wallet structure
./build/btc-recovery --analyze-wallet /path/to/wallet.dat

# 3. Quick recovery with common passwords
./build/btc-recovery --config config/wallet_dat_recovery.yaml --preset quick --wallet /path/to/wallet.dat

# 4. Comprehensive recovery (dictionary + brute force)
./build/btc-recovery --config config/wallet_dat_recovery.yaml --preset comprehensive --wallet /path/to/wallet.dat

# 5. If you know the password, just extract keys and check balances
./build/btc-recovery --wallet /path/to/wallet.dat --password "your_password" --extract-keys
```

### 🎯 Key Features for wallet.dat Recovery
- ✅ **No blockchain download** - Uses blockchain APIs for balance checking
- ✅ **Private key extraction** - Exports keys in multiple formats
- ✅ **Real-time balance checking** - Shows which addresses have funds
- ✅ **Multiple export formats** - Text, JSON, CSV, Electrum-compatible
- ✅ **Secure processing** - Works offline, only connects for balance checks

## 📦 Installation

### System Requirements
- **OS**: Linux (Ubuntu 18.04+), macOS 10.14+, Windows 10+
- **CPU**: Multi-core processor (4+ cores recommended)
- **RAM**: 4GB minimum, 8GB+ recommended
- **GPU**: Optional - NVIDIA (CUDA), AMD/Intel (OpenCL), or integrated graphics
- **Network**: Internet connection for blockchain API balance checking

### Build Options

#### Standard Build (CPU Only)
```bash
git clone https://github.com/vishwamartur/btc_recovery.git
cd btc_recovery
./scripts/build.sh
```

#### GPU-Accelerated Build
```bash
# With CUDA support (NVIDIA GPUs)
ENABLE_CUDA=1 ./scripts/build.sh

# With OpenCL support (AMD/Intel GPUs)
ENABLE_OPENCL=1 ./scripts/build.sh

# With both CUDA and OpenCL
ENABLE_CUDA=1 ENABLE_OPENCL=1 ./scripts/build.sh
```

#### Docker Build
```bash
# Build Docker image
docker build -t btc-recovery .

# Run with wallet.dat file
docker run -v /path/to/wallet:/data btc-recovery --wallet /data/wallet.dat
```

## 💡 Usage Examples

### Basic Password Recovery
```bash
# Test common passwords
./btc-recovery --wallet wallet.dat --passwords "password,123456,bitcoin,wallet"

# Dictionary attack
./btc-recovery --wallet wallet.dat --dictionary common_passwords.txt

# Brute force (short passwords)
./btc-recovery --wallet wallet.dat --charset lowercase --min-length 6 --max-length 8
```

### Advanced Recovery Options
```bash
# GPU-accelerated recovery
./btc-recovery --wallet wallet.dat --gpu --charset mixed --max-length 10

# Cluster recovery across multiple machines
./btc-recovery --wallet wallet.dat --cluster --node-id 0 --total-nodes 4

# Custom password patterns
./btc-recovery --wallet wallet.dat --prefix "bitcoin" --suffix "123" --charset digits
```

### API Configuration for Balance Checking
```bash
# Set API keys for higher rate limits
export BLOCKCYPHER_API_KEY="your-api-key-here"
export BLOCKCHAIR_API_KEY="your-api-key-here"

# Run recovery with API keys
./btc-recovery --wallet wallet.dat --config config/wallet_dat_recovery.yaml
```

## ⚡ Performance

### CPU Performance (Typical)
| CPU Type | Passwords/Second |
|----------|------------------|
| 4-core CPU | 10,000-50,000 |
| 8-core CPU | 20,000-100,000 |
| 16-core CPU | 40,000-200,000 |

### GPU Performance (Typical)
| GPU Type | Passwords/Second |
|----------|------------------|
| **Discrete GPUs** | |
| GTX 1060 | 100,000-500,000 |
| RTX 3070 | 500,000-2,000,000 |
| RTX 4090 | 1,000,000-5,000,000 |
| **Integrated Graphics** | |
| Intel HD Graphics | 5,000-20,000 |
| Intel Iris Graphics | 15,000-50,000 |
| AMD Vega APU | 20,000-80,000 |
| NVIDIA Tegra Orin | 50,000-200,000 |

*Performance varies based on wallet type, password complexity, and system configuration.*

### Optimization Tips
- **Use GPU acceleration** for 10-100x performance improvement
- **Enable cluster mode** for distributed processing
- **Start with dictionary attacks** before brute force
- **Use integrated graphics** on laptops for power efficiency
- **Configure API keys** to avoid rate limiting during balance checks

## 🔄 Continuous Integration

The project uses comprehensive GitHub Actions workflows for automated testing, quality assurance, and deployment:

### CI/CD Pipeline
- **Multi-platform builds**: Ubuntu 20.04/22.04, Windows 2022, macOS 12
- **Multiple compilers**: GCC, Clang, MSVC
- **GPU testing**: Both CUDA and OpenCL configurations
- **Automated testing**: Unit tests with coverage reporting
- **Dependency caching**: Faster builds with intelligent caching

### Code Quality Assurance
- **Static analysis**: cppcheck, clang-tidy, include-what-you-use
- **Code formatting**: clang-format with consistent style
- **Security scanning**: CodeQL and Semgrep vulnerability detection
- **License compliance**: Automated license header verification

### Automated Releases
- **Multi-platform binaries**: Linux, Windows, macOS releases
- **Docker images**: Multi-architecture container builds
- **Release automation**: Automatic changelog and artifact generation
- **Version management**: Semantic versioning with Git tags

### Documentation Pipeline
- **API documentation**: Automated Doxygen generation
- **Link validation**: Markdown link checking and badge verification
- **GitHub Pages**: Automatic documentation deployment
- **Format validation**: Markdown linting and style checking

## 📚 Documentation

### Complete Guides
- **[Setup Guide](docs/SETUP.md)** - Detailed installation and configuration
- **[Project Overview](PROJECT_OVERVIEW.md)** - Architecture and technical details
- **[Contributing Guide](CONTRIBUTING.md)** - How to contribute to the project

### Configuration Files
- **[recovery.yaml](config/recovery.yaml)** - General recovery configuration
- **[wallet_dat_recovery.yaml](config/wallet_dat_recovery.yaml)** - Bitcoin Core wallet.dat specific
- **[gpu.yaml](config/gpu.yaml)** - GPU acceleration settings
- **[integrated_gpu_presets.yaml](config/integrated_gpu_presets.yaml)** - Integrated graphics presets

### Example Programs
- **[basic_recovery.cpp](examples/basic_recovery.cpp)** - Simple recovery example
- **[wallet_dat_recovery.cpp](examples/wallet_dat_recovery.cpp)** - Complete wallet.dat recovery
- **[integrated_gpu_recovery.cpp](examples/integrated_gpu_recovery.cpp)** - Integrated graphics usage

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

### Development Setup
```bash
# Fork and clone the repository
git clone https://github.com/your-username/btc_recovery.git
cd btc_recovery

# Install development dependencies
sudo apt-get install -y build-essential cmake libssl-dev libcurl4-openssl-dev libjsoncpp-dev

# Build and test
./scripts/build.sh
cd build && ctest --output-on-failure
```

### Areas for Contribution
- Additional wallet format support
- Performance optimizations
- GPU kernel improvements
- Documentation and examples
- Testing and bug reports

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

### Important Legal Disclaimers
- **Legitimate Use Only**: This software is intended for recovering your own wallets
- **Legal Compliance**: Users must comply with local laws and regulations
- **No Warranty**: This software is provided as-is without guarantees
- **Security**: Always backup wallet files before attempting recovery

## 🔒 Security

### Responsible Use
- Only use this software to recover wallets you own or have explicit authorization to recover
- Never use this software for unauthorized access attempts
- Comply with all applicable computer crime laws in your jurisdiction

### Security Features
- **Offline Operation**: Private keys never leave your system
- **Secure Memory Handling**: Sensitive data is properly cleared
- **No Network Transmission**: Only balance checking connects to internet
- **Multiple Export Formats**: Redundant recovery data storage

### Reporting Security Issues
If you discover a security vulnerability, please report it privately to the maintainers.

## 🙏 Acknowledgments

- **BitPay Recovery Methodology**: Inspiration for lightweight wallet.dat recovery
- **Bitcoin Core**: Reference implementation for wallet format understanding
- **OpenSSL**: Cryptographic operations
- **CUDA/OpenCL**: GPU acceleration frameworks
- **Blockchain APIs**: Blockstream.info, Blockchair.com, BlockCypher.com

## 📞 Support

- **GitHub Issues**: [Report bugs and request features](https://github.com/vishwamartur/btc_recovery/issues)
- **Documentation**: Check the `docs/` directory for detailed guides
- **Examples**: Review the `examples/` directory for usage patterns

---

**⚠️ Remember**: This tool is designed for legitimate wallet recovery only. Always ensure you have legal authorization to recover any wallet files you process.

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
