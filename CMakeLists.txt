############################################################
# General configurations
############################################################
cmake_minimum_required(VERSION 3.1)

project(cbg)

# Pick the C++ standard to compile with.
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD 14)

############################################################
# Create a library
############################################################

add_library(cbg INTERFACE)
target_include_directories(cbg INTERFACE ${PROJECT_SOURCE_DIR})

############################################################
# Create a benchmark executable
############################################################

# Source
set(comparison_SOURCES comparison/compare_bench.cpp)

# Add an executable with the above sources
add_executable(comparison ${comparison_SOURCES})

# Add Abseil
add_subdirectory(comparison/abseil-cpp)
# Add Google Benchmark
add_subdirectory(comparison/benchmark)
# link
target_link_libraries(comparison PRIVATE cbg absl::flat_hash_map absl::flat_hash_set benchmark benchmark_main)

############################################################
# Testing
############################################################

# ...TODO...