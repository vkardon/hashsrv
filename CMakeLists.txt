cmake_minimum_required(VERSION 3.16)
project(MyBigSolution)

# Global Settings (Inherited by all sub-projects)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Add the sub-projects
add_subdirectory(client)
add_subdirectory(server)

