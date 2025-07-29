# Test configuration for Bitcoin Wallet Recovery System

cmake_minimum_required(VERSION 3.15)

# Find testing framework
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    # Download and build Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50f33f9142fd2d6828d5b901909825.zip
    )
    
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Test sources
set(TEST_SOURCES
    test_config_manager.cpp
    test_password_generator.cpp
    test_wallet_detection.cpp
    test_crypto_utils.cpp
)

# Create test executable
add_executable(btc_recovery_tests
    ${TEST_SOURCES}
    ../src/core/config_manager.cpp
    ../src/utils/logger.cpp
)

# Link libraries
target_link_libraries(btc_recovery_tests
    gtest_main
    gtest
    ${CMAKE_THREAD_LIBS_INIT}
    ${OPENSSL_LIBRARIES}
)

# Include directories
target_include_directories(btc_recovery_tests PRIVATE
    ../include
    ${OPENSSL_INCLUDE_DIR}
)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(btc_recovery_tests)
