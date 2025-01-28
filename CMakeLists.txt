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
include(ExternalProject)

set(BRPC_INSTALL_DIR ${CMAKE_BINARY_DIR}/brpc)
ExternalProject_Add(
    BRPC
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/third-party/brpc
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${BRPC_INSTALL_DIR}
        -DCMAKE_BUILD_TYPE=Release
)

set(SPDK_INSTALL_DIR ${CMAKE_BINARY_DIR}/spdk)
set(SPDK_SOURCE_DIR ${CMAKE_SOURCE_DIR}/third-party/spdk)
set(SPDK_MAKE cd ${SPDK_SOURCE_DIR} && make -j)
set(SPDK_INSTALL cd ${SPDK_SOURCE_DIR} && make install)
ExternalProject_Add(
    SPDK
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/third-party/spdk
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=${SPDK_INSTALL_DIR} --enable-debug
    BUILD_COMMAND ${SPDK_MAKE}
    INSTALL_COMMAND ${SPDK_INSTALL}
)
# TODO(xgj): fixme

# -----------------------PROTOBUF-----------------------
include(FindThreads)
include(FindProtobuf)
protobuf_generate_cpp(ECHO_PROTO_SRC PROTO_HEADER src/protobuf/echo.proto)
protobuf_generate_cpp(IO_PROTO_SRC PROTO_HEADER src/protobuf/io.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})


# -----------------------BRPC---------------------------
set(CMAKE_PREFIX_PATH ${BRPC_INSTALL_DIR})
set(BRPC_INCLUDE_PATH ${BRPC_INSTALL_DIR}/include)
set(BRPC_LIB ${BRPC_INSTALL_DIR}/lib/libbrpc.a)
include_directories(${BRPC_INCLUDE_PATH})

find_path(GFLAGS_INCLUDE_PATH gflags/gflags.h)
find_library(GFLAGS_LIBRARY NAMES gflags libgflags)
if((NOT GFLAGS_INCLUDE_PATH) OR (NOT GFLAGS_LIBRARY))
    message(FATAL_ERROR "Fail to find gflags")
endif()
include_directories(${GFLAGS_INCLUDE_PATH})

find_package(OpenSSL)
include_directories(${OPENSSL_INCLUDE_DIR})

find_path(LEVELDB_INCLUDE_PATH NAMES leveldb/db.h)
find_library(LEVELDB_LIB NAMES leveldb)
if ((NOT LEVELDB_INCLUDE_PATH) OR (NOT LEVELDB_LIB))
    message(FATAL_ERROR "Fail to find leveldb")
endif()
include_directories(${LEVELDB_INCLUDE_PATH})

set(BRPC_DYNAMIC_LIB
    ${CMAKE_THREAD_LIBS_INIT}
    ${GFLAGS_LIBRARY}
    ${PROTOBUF_LIBRARIES}
    ${LEVELDB_LIB}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
    dl
)
# ------------------Internal Files---------------------------
include_directories(${CMAKE_SOURCE_DIR}/src/include)
file(GLOB MCBS_SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.cc)

# ------------------Targets----------------------------------
add_executable(client_tgt app/client_tgt.cc ${IO_PROTO_SRC} ${IO_PROTO_HEADER} ${MCBS_SRC_FILES})
add_executable(server_tgt app/server_tgt.cc ${IO_PROTO_SRC} ${IO_PROTO_HEADER} ${MCBS_SRC_FILES})
add_dependencies(client_tgt BRPC SPDK)
add_dependencies(server_tgt BRPC SPDK)

target_link_libraries(client_tgt ${BRPC_LIB} ${BRPC_DYNAMIC_LIB})
target_link_libraries(server_tgt ${BRPC_LIB} ${BRPC_DYNAMIC_LIB})