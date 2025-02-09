find_package(PkgConfig REQUIRED)
if(NOT PKG_CONFIG_FOUND)
    message(FATAL_ERROR "pkg-config command not found!" )
endif()

set(SPDK_BUILD_DIR "${CMAKE_SOURCE_DIR}/third-party/spdk/build")

# Needed to ensure that PKG_CONFIG also looks at our SPDK installation.
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${SPDK_BUILD_DIR}/lib/pkgconfig/")
message("Looking for SPDK packages...")
pkg_check_modules(SPDK_LIB REQUIRED IMPORTED_TARGET spdk_event spdk_event_bdev spdk_env_dpdk)

if(SPDK_LIB_FOUND)
    list(TRANSFORM SPDK_LIB_LINK_LIBRARIES REPLACE "[.]so$" ".a")
    message(STATUS "SPDK LIB found")
    message(STATUS "Include dirs: ${SPDK_LIB_INCLUDE_DIRS}")
    message(STATUS "Libraries: ${SPDK_LIB_LIBRARIES}")
else()
    message(FATAL_ERROR "SPDK LIB not found")
endif()

# fixme(xgj): This is a hack to get the static libraries from SPDK cause pkg_xxx_module(s) can't just work
execute_process(
    COMMAND pkg-config --libs --static spdk_syslibs
    OUTPUT_VARIABLE PKG_CONFIG_SYS_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

separate_arguments(SYS_STATIC_LINK_LIBRARIES NATIVE_COMMAND "${PKG_CONFIG_SYS_OUTPUT}")

set(SPDK_STATIC_LIB
    -Wl,--whole-archive
    "${SPDK_LIB_LINK_LIBRARIES}"
    -Wl,--no-whole-archive
    "${SYS_STATIC_LINK_LIBRARIES}" 
)

set(SPDK_INCLUDE_DIRS
    "${SPDK_LIB_INCLUDE_DIRS}"
)