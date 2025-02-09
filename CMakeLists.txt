cmake_minimum_required(VERSION 3.16)
project(simple-zns-device VERSION 1.0.1 DESCRIPTION "A Migrating-Based Cloud Block Storage System")
message(STATUS "Building using CMake version: ${CMAKE_VERSION}")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message("compiler is ${CMAKE_CXX_COMPILER_ID} and name is ${CMAKE_CXX_COMPILER}")

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${CMAKE_LIB_DIR}")

# Default to debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
# Set flags
set(BUILD_FLAGS -Wall -Wextra -Werror -std=c++11)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    list(APPEND BUILD_FLAGS -g2 -ggdb -O0)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()
if (CMAKE_BUILD_TYPE MATCHES Release)
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


# -----------------------third-party---------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(FindBRPC)
include(FindSPDK)

# -----------------------PROTOBUF-----------------------
protobuf_generate_cpp(ECHO_PROTO_SRC PROTO_HEADER src/protobuf/echo.proto)
protobuf_generate_cpp(IO_PROTO_SRC PROTO_HEADER src/protobuf/io.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# ------------------MCBS LIBRARY---------------------------
file(GLOB MCBS_SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.cc)
add_library(mcbs_lib STATIC ${MCBS_SRC_FILES} ${IO_PROTO_SRC} ${IO_PROTO_HEADER})
target_include_directories(mcbs_lib PUBLIC ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(mcbs_lib PUBLIC ${SPDK_INCLUDE_DIRS})
target_link_libraries(mcbs_lib PRIVATE ${BRPC_LIB} ${BRPC_DYNAMIC_LIB} ${SPDK_STATIC_LIB})
add_dependencies(mcbs_lib BRPC)

# ------------------Targets----------------------------------
add_executable(client_tgt app/client_tgt.cc)
add_executable(server_tgt app/server_tgt.cc)

target_link_libraries(client_tgt PRIVATE mcbs_lib)
target_link_libraries(server_tgt PRIVATE mcbs_lib ${SPDK_STATIC_LIB})
target_include_directories(server_tgt PUBLIC ${SPDK_INCLUDE_DIRS})