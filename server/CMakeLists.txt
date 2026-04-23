cmake_minimum_required(VERSION 3.16)
project(HashServer VERSION 1.0 LANGUAGES CXX)

# Configure C++ Standard (Matches -std=gnu++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Find Dependencies: libssl libcrypto and pthread
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Asio is header-only on Ubuntu, but we need to find its location
find_path(ASIO_INCLUDE_DIR asio.hpp)

# Define the Executable
add_executable(server_asio 
    src/main.cpp
    # Add other source files here
)

# Include Directories
target_include_directories(server_asio PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ASIO_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

# Link Libraries: libssl, libcrypto, pthread
target_link_libraries(server_asio PRIVATE 
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

# Compiler Flags: -Wall
target_compile_options(server_asio PRIVATE -Wall)

# CMake handles Debug/Release automatically via CMAKE_BUILD_TYPE.
# If CMAKE_BUILD_TYPE is "Release", it adds -O3 and -DNDEBUG.
# If "Debug", it adds -g.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# --- Testing Section ---
enable_testing()

# Optional: Helping CMake find GTest on RHEL/Alma/Fedora
# set(GTEST_ROOT /usr/src/gtest)
find_package(GTest REQUIRED)

add_executable(session_test tests/sessionTest.cpp)

# Link everything session_test needs
target_link_libraries(session_test PRIVATE 
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    GTest::gtest_main
)

# Ensure the test can find your session.hpp
target_include_directories(session_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ASIO_INCLUDE_DIR}
)

add_test(NAME SessionLogicTest COMMAND session_test)

# --- Server Integration Test ---
add_executable(server_test tests/serverTest.cpp)

target_link_libraries(server_test PRIVATE 
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    GTest::gtest_main
)

target_include_directories(server_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ASIO_INCLUDE_DIR}
)

add_test(NAME ServerIntegrationTest COMMAND server_test)



