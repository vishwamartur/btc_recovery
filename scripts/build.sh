#!/bin/bash

# Bitcoin Wallet Recovery Build Script

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_TYPE=${BUILD_TYPE:-Release}
BUILD_DIR=${BUILD_DIR:-build}
INSTALL_PREFIX=${INSTALL_PREFIX:-/usr/local}
ENABLE_GPU=${ENABLE_GPU:-ON}
ENABLE_CUDA=${ENABLE_CUDA:-ON}
ENABLE_OPENCL=${ENABLE_OPENCL:-ON}
BUILD_TESTS=${BUILD_TESTS:-ON}
BUILD_EXAMPLES=${BUILD_EXAMPLES:-ON}
PARALLEL_JOBS=${PARALLEL_JOBS:-$(nproc)}

echo -e "${BLUE}Bitcoin Wallet Recovery Build Script${NC}"
echo -e "${BLUE}====================================${NC}"

# Function to print status messages
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    # Check for required tools
    command -v cmake >/dev/null 2>&1 || { print_error "cmake is required but not installed."; exit 1; }
    command -v make >/dev/null 2>&1 || { print_error "make is required but not installed."; exit 1; }
    command -v g++ >/dev/null 2>&1 || { print_error "g++ is required but not installed."; exit 1; }
    
    # Check for OpenSSL
    if ! pkg-config --exists openssl; then
        print_error "OpenSSL development libraries are required but not found."
        print_error "Install with: sudo apt-get install libssl-dev (Ubuntu/Debian)"
        print_error "            or: sudo yum install openssl-devel (CentOS/RHEL)"
        exit 1
    fi
    
    # Check for CUDA (optional)
    if [[ "$ENABLE_CUDA" == "ON" ]]; then
        if command -v nvcc >/dev/null 2>&1; then
            print_status "CUDA compiler found: $(nvcc --version | grep release)"
        else
            print_warning "CUDA compiler not found. GPU acceleration will be disabled."
            ENABLE_CUDA=OFF
        fi
    fi
    
    # Check for OpenCL (optional)
    if [[ "$ENABLE_OPENCL" == "ON" ]]; then
        if pkg-config --exists OpenCL; then
            print_status "OpenCL found"
        else
            print_warning "OpenCL not found. OpenCL support will be disabled."
            ENABLE_OPENCL=OFF
        fi
    fi
    
    print_status "Dependency check completed"
}

# Clean build directory
clean_build() {
    if [[ "$1" == "clean" ]]; then
        print_status "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
    fi
}

# Configure build
configure_build() {
    print_status "Configuring build..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    cmake .. \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DENABLE_GPU="$ENABLE_GPU" \
        -DENABLE_CUDA="$ENABLE_CUDA" \
        -DENABLE_OPENCL="$ENABLE_OPENCL" \
        -DBUILD_TESTS="$BUILD_TESTS" \
        -DBUILD_EXAMPLES="$BUILD_EXAMPLES"
    
    cd ..
    print_status "Configuration completed"
}

# Build project
build_project() {
    print_status "Building project with $PARALLEL_JOBS parallel jobs..."
    
    cd "$BUILD_DIR"
    make -j"$PARALLEL_JOBS"
    cd ..
    
    print_status "Build completed successfully"
}

# Run tests
run_tests() {
    if [[ "$BUILD_TESTS" == "ON" ]]; then
        print_status "Running tests..."
        cd "$BUILD_DIR"
        ctest --output-on-failure
        cd ..
        print_status "Tests completed"
    fi
}

# Install project
install_project() {
    if [[ "$1" == "install" ]]; then
        print_status "Installing project..."
        cd "$BUILD_DIR"
        sudo make install
        cd ..
        print_status "Installation completed"
    fi
}

# Print build summary
print_summary() {
    echo
    echo -e "${GREEN}Build Summary:${NC}"
    echo -e "  Build Type: $BUILD_TYPE"
    echo -e "  Build Directory: $BUILD_DIR"
    echo -e "  GPU Support: $ENABLE_GPU"
    echo -e "  CUDA Support: $ENABLE_CUDA"
    echo -e "  OpenCL Support: $ENABLE_OPENCL"
    echo -e "  Tests: $BUILD_TESTS"
    echo -e "  Examples: $BUILD_EXAMPLES"
    echo -e "  Parallel Jobs: $PARALLEL_JOBS"
    echo
    echo -e "${GREEN}Executable location: $BUILD_DIR/btc-recovery${NC}"
    echo
}

# Main execution
main() {
    check_dependencies
    clean_build "$1"
    configure_build
    build_project
    run_tests
    install_project "$1"
    print_summary
}

# Handle command line arguments
case "$1" in
    clean)
        main clean
        ;;
    install)
        main install
        ;;
    help|--help|-h)
        echo "Usage: $0 [clean|install|help]"
        echo
        echo "Options:"
        echo "  clean    - Clean build directory before building"
        echo "  install  - Install after building (requires sudo)"
        echo "  help     - Show this help message"
        echo
        echo "Environment variables:"
        echo "  BUILD_TYPE       - Build type (Debug|Release) [default: Release]"
        echo "  BUILD_DIR        - Build directory [default: build]"
        echo "  INSTALL_PREFIX   - Install prefix [default: /usr/local]"
        echo "  ENABLE_GPU       - Enable GPU support [default: ON]"
        echo "  ENABLE_CUDA      - Enable CUDA support [default: ON]"
        echo "  ENABLE_OPENCL    - Enable OpenCL support [default: ON]"
        echo "  BUILD_TESTS      - Build tests [default: ON]"
        echo "  BUILD_EXAMPLES   - Build examples [default: ON]"
        echo "  PARALLEL_JOBS    - Number of parallel jobs [default: nproc]"
        ;;
    *)
        main
        ;;
esac
