# Override to use different location for SPDK.
if(DEFINED ENV{BRPC_DIR})
    set(BRPC_DIR "$ENV{BRPC_DIR}")
else()
    set(BRPC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third-party/brpc")
endif()
message("looking for BRPC in ${BRPC_DIR}")


add_subdirectory(${BRPC_DIR})