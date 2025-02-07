include(ExternalProject)

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

find_package(PkgConfig REQUIRED)
if(NOT PKG_CONFIG_FOUND)
    message(FATAL_ERROR "pkg-config command not found!" )
endif()

# Needed to ensure that PKG_CONFIG also looks at our SPDK installation.
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${SPDK_INSTALL_DIR}/lib/pkgconfig/")
message("Looking for SPDK packages...")
pkg_search_module(SPDK_FTL REQUIRED IMPORTED_TARGET spdk_ftl)
pkg_search_module(DPDK REQUIRED IMPORTED_TARGET spdk_env_dpdk)

if(SPDK_FTL_FOUND)
    message(STATUS "SPDK found")
    message(STATUS "Include dirs: ${SPDK_INCLUDE_DIRS}")
    message(STATUS "Libraries: ${SPDK_LIBRARIES}")
else()
    message(FATAL_ERROR "SPDK not found")
endif()

if(DPDK_FOUND)
    message(STATUS "DPDK found")
    message(STATUS "Include dirs: ${DPDK_INCLUDE_DIRS}")
    message(STATUS "Libraries: ${DPDK_LIBRARIES}")
else()
    message(FATAL_ERROR "DPDK not found")
endif()

set(SPDK_STATIC_LIB
    -Wl,--whole-archive
    "${SPDK_LINK_LIBRARIES}"
    -Wl,--no-whole-archive
    "${DPDK_LINK_LIBRARIES}" 
    "${SYS_STATIC_LIBRARIES}" 
)

set(SPDK_INCLUDE_DIRS
    "${SPDK_FTL_INCLUDE_DIRS}"
)
include_directories(${SPDK_INCLUDE_DIRS})