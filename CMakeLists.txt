cmake_minimum_required(VERSION 3.13)
project(hw5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER "g++")

set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O3 -g -march=skylake-avx512")

include(lib/gtest.cmake)

include_directories(include)

add_executable(sync src/main.cpp)
target_link_libraries(sync tbb)

### Tests
add_executable(test_all test/synchronization.cpp)
target_link_libraries(test_all tbb gtest gtest_main pthread)
