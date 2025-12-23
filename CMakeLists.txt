cmake_minimum_required(VERSION 3.20)
project(hybrid_map LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include.
add_library(hybrid_map INTERFACE)
target_include_directories(hybrid_map INTERFACE
  ${PROJECT_SOURCE_DIR}/include
)

# Example.
add_executable(examples src/examples.cpp)
target_link_libraries(examples PRIVATE hybrid_map)

# Google Test.
find_package(GTest QUIET)
if(NOT GTest_FOUND)
  message(STATUS "Google Test not found.")
else()
  message(STATUS "Found Google Test.")
  enable_testing()
  add_subdirectory(tests)
endif()

# Google Benchmark.
find_package(benchmark QUIET)
if(NOT benchmark_FOUND)
  message(STATUS "Google Benchmark not found.")
else()
  message(STATUS "Found Google Benchmark")
  add_subdirectory(benches)
endif()
