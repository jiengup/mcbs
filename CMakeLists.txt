cmake_minimum_required(VERSION 3.16)
project(simple-zns-device VERSION 1.0.1 DESCRIPTION "A Migrating-Based Cloud Block Storage System")
message(STATUS "Building using CMake version: ${CMAKE_VERSION}")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message("compiler is ${CMAKE_CXX_COMPILER_ID} and name is ${CMAKE_CXX_COMPILER}")

set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIB_DIR "${CMAKE_SOURCE_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(LIBRARY_OUTPUT_PATH "${CMAKE_LIB_DIR}")

# Default to debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
# Set flags
set(BUILD_FLAGS -Wall -Wextra -Werror)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    message("Building SZD in Debug mode")
    list(APPEND BUILD_FLAGS -g2 -ggdb -O0)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()
if (CMAKE_BUILD_TYPE MATCHES Release)
    message("Building SZD in Release mode")
    list(APPEND BUILD_FLAGS -O3)
endif()
option(FSANITIZE "Turn on fsanitize" OFF)
if (FSANITIZE)
    list(APPEND BUILD_FLAGS
        -fsanitize=address #ASAN
        -fsanitize=undefined  #UBSAN
        -fsanitize=float-divide-by-zero
        -fsanitize=float-cast-overflow
        -fno-sanitize-recover=all
        -fno-sanitize=alignment
    )
    message("fsanitize is turned on")
endif()

# Sets up SPDK
# include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindSPDK.cmake")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
find_package(BRPC)