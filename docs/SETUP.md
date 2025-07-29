# Bitcoin Wallet Recovery System - Setup Guide

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Dependencies](#dependencies)
3. [Building from Source](#building-from-source)
4. [Configuration](#configuration)
5. [GPU Setup](#gpu-setup)
6. [Cluster Deployment](#cluster-deployment)
7. [Troubleshooting](#troubleshooting)

## System Requirements

### Minimum Requirements
- **CPU**: Multi-core processor (4+ cores recommended)
- **RAM**: 4GB minimum, 8GB+ recommended
- **Storage**: 1GB free space for application and logs
- **OS**: Linux (Ubuntu 18.04+, CentOS 7+), macOS 10.14+, Windows 10+

### Recommended for High Performance
- **CPU**: Intel Xeon or AMD EPYC with 16+ cores
- **RAM**: 32GB+ for large-scale operations
- **GPU**: NVIDIA GPU with CUDA Compute Capability 5.0+ (GTX 1060 or better)
- **Storage**: SSD for faster I/O operations

## Dependencies

### Core Dependencies
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    pkg-config \
    libboost-all-dev

# CentOS/RHEL
sudo yum groupinstall -y "Development Tools"
sudo yum install -y \
    cmake3 \
    git \
    openssl-devel \
    pkgconfig \
    boost-devel

# macOS (with Homebrew)
brew install cmake openssl boost pkg-config
```

### Optional GPU Dependencies

#### CUDA (NVIDIA GPUs)
```bash
# Download and install CUDA Toolkit from NVIDIA
wget https://developer.download.nvidia.com/compute/cuda/11.8.0/local_installers/cuda_11.8.0_520.61.05_linux.run
sudo sh cuda_11.8.0_520.61.05_linux.run

# Add to PATH
echo 'export PATH=/usr/local/cuda/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
```

#### OpenCL
```bash
# Ubuntu/Debian
sudo apt-get install -y opencl-headers ocl-icd-opencl-dev

# CentOS/RHEL
sudo yum install -y opencl-headers ocl-icd-devel

# For Intel CPUs
sudo apt-get install -y intel-opencl-icd

# For AMD GPUs
sudo apt-get install -y mesa-opencl-icd
```

## Building from Source

### Quick Build
```bash
# Clone the repository
git clone https://github.com/your-repo/btc-recovery.git
cd btc-recovery

# Build with default settings
./scripts/build.sh
```

### Custom Build Options
```bash
# Clean build with specific options
BUILD_TYPE=Release \
ENABLE_GPU=ON \
ENABLE_CUDA=ON \
ENABLE_OPENCL=ON \
BUILD_TESTS=ON \
./scripts/build.sh clean

# Debug build
BUILD_TYPE=Debug ./scripts/build.sh

# Build without GPU support
ENABLE_GPU=OFF ./scripts/build.sh
```

### Manual Build
```bash
mkdir build && cd build

# Configure
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_GPU=ON \
    -DENABLE_CUDA=ON \
    -DENABLE_OPENCL=ON \
    -DBUILD_TESTS=ON

# Build
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Install (optional)
sudo make install
```

## Configuration

### Basic Configuration
Create a configuration file based on the template:
```bash
cp config/recovery.yaml my_recovery.yaml
```

Edit the configuration:
```yaml
# Wallet settings
wallet_file: "/path/to/your/wallet.dat"
wallet_type: "auto"

# Password generation
charset: "mixed"
min_length: 6
max_length: 12
prefix: ""
suffix: ""

# Performance
threads: 8
batch_size: 10000
use_gpu: true
```

### Advanced Configuration Options

#### Character Sets
- `lowercase`: a-z
- `uppercase`: A-Z
- `digits`: 0-9
- `symbols`: !@#$%^&*()_+-=[]{}|;:,.<>?
- `mixed`: All of the above
- `custom`: Define your own in `custom_charset`

#### Recovery Modes
- `brute_force`: Systematic password generation
- `dictionary`: Dictionary-based attack
- `hybrid`: Dictionary + transformations
- `gpu_only`: GPU-accelerated only

## GPU Setup

### NVIDIA GPU Setup
1. Install NVIDIA drivers:
```bash
# Ubuntu
sudo apt-get install -y nvidia-driver-470

# Or use the graphics-drivers PPA for latest
sudo add-apt-repository ppa:graphics-drivers/ppa
sudo apt-get update
sudo apt-get install -y nvidia-driver-latest
```

2. Install CUDA Toolkit (see Dependencies section)

3. Verify installation:
```bash
nvidia-smi
nvcc --version
```

4. Configure GPU settings in `config/gpu.yaml`:
```yaml
cuda:
  enabled: true
  device_id: 0
  threads_per_block: 256
  blocks_per_grid: 1024
```

### AMD GPU Setup (OpenCL)
1. Install AMD drivers and OpenCL:
```bash
# Ubuntu
sudo apt-get install -y mesa-opencl-icd

# For AMD proprietary drivers
wget https://repo.radeon.com/amdgpu-install/latest/ubuntu/focal/amdgpu-install_*_all.deb
sudo dpkg -i amdgpu-install_*_all.deb
sudo amdgpu-install --usecase=opencl
```

2. Verify OpenCL:
```bash
clinfo
```

## Integrated Graphics Setup

### Intel Integrated Graphics
1. Install Intel OpenCL runtime:
```bash
# Ubuntu/Debian
sudo apt-get install -y intel-opencl-icd

# For newer Intel Arc graphics
wget https://github.com/intel/compute-runtime/releases/latest/download/intel-gmmlib_*.deb
wget https://github.com/intel/compute-runtime/releases/latest/download/intel-igc-core_*.deb
wget https://github.com/intel/compute-runtime/releases/latest/download/intel-igc-opencl_*.deb
wget https://github.com/intel/compute-runtime/releases/latest/download/intel-opencl-icd_*.deb
sudo dpkg -i *.deb
```

2. Verify Intel GPU detection:
```bash
clinfo | grep Intel
```

3. Configure for Intel integrated graphics:
```yaml
# config/recovery.yaml
use_gpu: true
gpu_device: 0  # Usually the integrated GPU

# Use integrated GPU preset
integrated_gpu_preset: "intel.hd_graphics"  # or iris_graphics, arc_graphics
```

### AMD Integrated Graphics (APU)
1. Install AMD APU drivers:
```bash
# Ubuntu
sudo apt-get install -y mesa-opencl-icd

# For Ryzen APUs with newer RDNA graphics
sudo add-apt-repository ppa:oibaf/graphics-drivers
sudo apt-get update
sudo apt-get install -y mesa-opencl-icd
```

2. Verify AMD APU detection:
```bash
clinfo | grep AMD
lspci | grep VGA  # Should show AMD/ATI device
```

3. Configure for AMD integrated graphics:
```yaml
# config/recovery.yaml
use_gpu: true
integrated_gpu_preset: "amd.vega_apu"  # or rdna_apu
```

### NVIDIA Integrated Graphics (Tegra/Mobile)
1. Install NVIDIA drivers for integrated GPUs:
```bash
# For Jetson/Tegra devices
sudo apt-get install -y nvidia-cuda-toolkit

# For mobile NVIDIA GPUs
sudo apt-get install -y nvidia-driver-470
```

2. Verify CUDA integrated GPU:
```bash
nvidia-smi  # Should show integrated GPU
nvcc --version
```

3. Configure for NVIDIA integrated graphics:
```yaml
# config/recovery.yaml
use_gpu: true
gpu_device: 0
integrated_gpu_preset: "nvidia.tegra_x1"  # or tegra_x2, tegra_xavier, tegra_orin, mx_series
```

### Apple Silicon (M1/M2/M3)
**Note**: Apple deprecated OpenCL support. CUDA is not available on Apple Silicon.

1. For legacy OpenCL support (limited):
```bash
# Check if OpenCL is available
system_profiler SPDisplaysDataType | grep OpenCL
```

2. Recommended: Use CPU-only mode for Apple Silicon:
```yaml
# config/recovery.yaml
use_gpu: false
threads: 8  # Utilize all CPU cores
```

3. Alternative: Use Metal Performance Shaders (requires custom implementation):
```yaml
# Future support for Metal compute shaders
use_metal: true  # Not yet implemented
```

### Performance Tuning

#### Discrete GPU Tuning
- **Memory Usage**: Adjust `memory_pool_size` based on GPU memory
- **Batch Size**: Larger batches = better GPU utilization
- **Thread Configuration**: Optimize for your specific GPU architecture

#### Integrated Graphics Tuning
- **Memory Bandwidth Limited**: Reduce batch sizes and buffer sizes
- **Thermal Throttling**: Enable thermal monitoring and reduce clock speeds
- **Power Constraints**: Use power-efficient presets for laptops
- **Shared Memory**: Account for system memory usage by other applications

**Intel Integrated Graphics**:
```yaml
# Optimize for Intel HD Graphics
opencl:
  work_group_size: 64
  buffer_size: "128MB"
  batch_size: 1000
performance:
  cpu_gpu_ratio: 0.7  # Favor CPU over weak integrated GPU
  thermal_throttling: true
```

**AMD APU Graphics**:
```yaml
# Optimize for AMD Vega/RDNA APU
opencl:
  work_group_size: 128
  buffer_size: "256MB"
  batch_size: 2500
performance:
  cpu_gpu_ratio: 0.4  # Better GPU performance than Intel
  memory_usage_ratio: 0.6
```

**NVIDIA Tegra/Mobile**:
```yaml
# Optimize for NVIDIA Tegra
cuda:
  threads_per_block: 128
  blocks_per_grid: 64
  enable_unified_memory: true
  stream_count: 2
performance:
  thermal_throttling: true
  power_limit: 15  # watts
```

**Laptop-Specific Optimizations**:
- **Battery Mode**: Reduce performance by 50%, enable aggressive throttling
- **Balanced Mode**: Reduce performance by 20%, moderate throttling
- **Performance Mode**: Full performance, higher thermal limits

## Cluster Deployment

### Local Cluster Setup
1. Configure cluster settings in `config/cluster.yaml`
2. Start coordinator node:
```bash
./btc-recovery --config config/cluster.yaml --cluster-coordinator
```

3. Start worker nodes:
```bash
./btc-recovery --config config/cluster.yaml --cluster-worker --node-id 1
./btc-recovery --config config/cluster.yaml --cluster-worker --node-id 2
```

### AWS EC2 Deployment
1. Configure AWS credentials:
```bash
aws configure
```

2. Update deployment settings in `scripts/deploy_aws.sh`

3. Deploy cluster:
```bash
# Deploy 4 instances
INSTANCE_COUNT=4 ./scripts/deploy_aws.sh deploy

# Check status
./scripts/deploy_aws.sh status

# Terminate when done
./scripts/deploy_aws.sh terminate
```

### Docker Deployment
```bash
# Build Docker image
docker build -t btc-recovery .

# Run single container
docker run -v /path/to/wallet:/data btc-recovery \
    --wallet /data/wallet.dat --charset mixed

# Run with GPU support
docker run --gpus all -v /path/to/wallet:/data btc-recovery \
    --wallet /data/wallet.dat --gpu
```

## Troubleshooting

### Common Issues

#### Build Errors
```bash
# Missing OpenSSL
sudo apt-get install libssl-dev

# CMake version too old
wget https://cmake.org/files/v3.20/cmake-3.20.0-linux-x86_64.sh
sudo sh cmake-3.20.0-linux-x86_64.sh --prefix=/usr/local --skip-license

# CUDA not found
export CUDA_ROOT=/usr/local/cuda
export PATH=$CUDA_ROOT/bin:$PATH
```

#### Runtime Issues
```bash
# Permission denied on wallet file
chmod 644 /path/to/wallet.dat

# GPU not detected
nvidia-smi  # Check if GPU is visible
lsmod | grep nvidia  # Check if drivers are loaded

# Out of memory
# Reduce batch_size in configuration
# Reduce gpu_threads for GPU operations
```

#### Performance Issues
- **Low CPU utilization**: Increase `threads` and `batch_size`
- **Low GPU utilization**: Increase `gpu_threads` and `blocks_per_grid`
- **Memory issues**: Reduce `memory_pool_size` and batch sizes

### Logging and Debugging
```bash
# Enable debug logging
./btc-recovery --log-level debug --wallet wallet.dat

# Monitor system resources
htop
nvidia-smi -l 1  # For GPU monitoring

# Check log files
tail -f recovery.log
```

### Getting Help
1. Check the logs for error messages
2. Verify all dependencies are installed
3. Test with a simple configuration first
4. Check GPU compatibility and drivers
5. Review the examples in the `examples/` directory

For additional support, please refer to the project documentation or create an issue in the repository.
