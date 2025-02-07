include(ExternalProject)

set(BRPC_SOURCE_DIR ${CMAKE_SOURCE_DIR}/third-party/brpc)
set(BRPC_INSTALL_DIR ${CMAKE_BINARY_DIR}/brpc)
ExternalProject_Add(
    BRPC
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/third-party/brpc
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${BRPC_INSTALL_DIR}
        -DCMAKE_BUILD_TYPE=Release
)

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

include(FindThreads)
include(FindProtobuf)

set(BRPC_DYNAMIC_LIB
    ${CMAKE_THREAD_LIBS_INIT}
    ${GFLAGS_LIBRARY}
    ${PROTOBUF_LIBRARIES}
    ${LEVELDB_LIB}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
    dl
)