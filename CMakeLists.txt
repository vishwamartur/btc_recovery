cmake_minimum_required(VERSION 3.15)
project(btc-recovery VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -march=native")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -march=native")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /O2")
endif()

# Options
option(ENABLE_GPU "Enable GPU acceleration support" ON)
option(ENABLE_CUDA "Enable CUDA support" ON)
option(ENABLE_OPENCL "Enable OpenCL support" ON)
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build example programs" ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Optional GPU support
if(ENABLE_GPU)
    if(ENABLE_CUDA)
        find_package(CUDA QUIET)
        if(CUDA_FOUND)
            enable_language(CUDA)
            add_definitions(-DENABLE_CUDA)
            message(STATUS "CUDA support enabled")
        endif()
    endif()
    
    if(ENABLE_OPENCL)
        find_package(OpenCL QUIET)
        if(OpenCL_FOUND)
            add_definitions(-DENABLE_OPENCL)
            message(STATUS "OpenCL support enabled")
        endif()
    endif()
endif()

# Include directories
include_directories(include)
include_directories(${OPENSSL_INCLUDE_DIR})

# Source files
set(CORE_SOURCES
    src/core/recovery_engine.cpp
    src/core/password_generator.cpp
    src/core/thread_pool.cpp
    src/core/progress_tracker.cpp
    src/core/config_manager.cpp
)

set(WALLET_SOURCES
    src/wallets/wallet_base.cpp
    src/wallets/bitcoin_core_wallet.cpp
    src/wallets/electrum_wallet.cpp
    src/wallets/multibit_wallet.cpp
    src/wallets/bip38_handler.cpp
)

set(UTILS_SOURCES
    src/utils/crypto_utils.cpp
    src/utils/file_utils.cpp
    src/utils/string_utils.cpp
    src/utils/logger.cpp
)

set(GPU_SOURCES)
if(CUDA_FOUND)
    set(GPU_SOURCES ${GPU_SOURCES}
        src/gpu/cuda_recovery.cu
        src/gpu/cuda_utils.cu
        src/gpu/cuda_integrated.cpp
    )
endif()

if(OpenCL_FOUND)
    set(GPU_SOURCES ${GPU_SOURCES}
        src/gpu/opencl_recovery.cpp
        src/gpu/opencl_utils.cpp
        src/gpu/integrated_gpu.cpp
    )
endif()

# Main executable
add_executable(btc-recovery
    src/main.cpp
    ${CORE_SOURCES}
    ${WALLET_SOURCES}
    ${UTILS_SOURCES}
    ${GPU_SOURCES}
)

# Link libraries
target_link_libraries(btc-recovery
    ${CMAKE_THREAD_LIBS_INIT}
    ${OPENSSL_LIBRARIES}
)

if(CUDA_FOUND)
    target_link_libraries(btc-recovery ${CUDA_LIBRARIES})
endif()

if(OpenCL_FOUND)
    target_link_libraries(btc-recovery ${OpenCL_LIBRARIES})
endif()

# Set properties for CUDA files
if(CUDA_FOUND)
    # Include architectures for both discrete and integrated GPUs
    # 35, 37: Tegra K1, Jetson TK1
    # 50, 52: Maxwell (GTX 900 series, some mobile GPUs)
    # 53: Tegra X1, Jetson Nano, Shield TV
    # 60, 61, 62: Pascal (GTX 1000 series, Tegra X2, Jetson TX2)
    # 70, 72: Volta (Tegra Xavier, Jetson Xavier)
    # 75: Turing (GTX 1600/RTX 2000 series, some mobile GPUs)
    # 80, 86: Ampere (RTX 3000 series, some mobile GPUs)
    # 87: Tegra Orin, Jetson Orin
    # 89, 90: Ada Lovelace (RTX 4000 series)
    set_property(TARGET btc-recovery PROPERTY CUDA_ARCHITECTURES 35 37 50 52 53 60 61 62 70 72 75 80 86 87 89 90)

    # Set CUDA-specific compiler flags for integrated GPUs
    set_property(TARGET btc-recovery PROPERTY CUDA_COMPILE_OPTIONS
        $<$<COMPILE_LANGUAGE:CUDA>:-use_fast_math>
        $<$<COMPILE_LANGUAGE:CUDA>:-lineinfo>
        $<$<COMPILE_LANGUAGE:CUDA>:-Xptxas=-v>
    )
endif()

# Install targets
install(TARGETS btc-recovery DESTINATION bin)
install(DIRECTORY config/ DESTINATION share/btc-recovery/config)
install(FILES README.md DESTINATION share/btc-recovery)

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  GPU Support: ${ENABLE_GPU}")
if(ENABLE_GPU)
    message(STATUS "    CUDA: ${CUDA_FOUND}")
    message(STATUS "    OpenCL: ${OpenCL_FOUND}")
endif()
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "  Build Examples: ${BUILD_EXAMPLES}")
message(STATUS "")
