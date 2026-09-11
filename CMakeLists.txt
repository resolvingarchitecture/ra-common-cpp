cmake_minimum_required(VERSION 3.16)
project(ra_common LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Header-only library.
add_library(ra_common INTERFACE)
target_include_directories(ra_common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)
target_compile_features(ra_common INTERFACE cxx_std_20)

option(RA_COMMON_BUILD_TESTS "Build ra-common-cpp tests" ON)
if(RA_COMMON_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
