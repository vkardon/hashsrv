cmake_minimum_required(VERSION 3.16)
project(HashServer VERSION 1.0 LANGUAGES CXX)

# Configure C++ Standard (Matches -std=gnu++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Find Dependencies: pthread
find_package(Threads REQUIRED)

# Define the Executable
add_executable(client
    client.cpp
    # Add other source files here
)

# Include Directories
target_include_directories(client PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    # Add other directories here
)

# Link Libraries 
target_link_libraries(client PRIVATE 
    # Add other libraries here
)

# Compiler Flags: -Wall
target_compile_options(client PRIVATE -Wall)

# CMake handles Debug/Release automatically via CMAKE_BUILD_TYPE.
# If CMAKE_BUILD_TYPE is "Release", it adds -O3 and -DNDEBUG.
# If "Debug", it adds -g.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()


