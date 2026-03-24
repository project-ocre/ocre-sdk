cmake_minimum_required(VERSION 3.20.0)

project(ocre-sdk)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

if (NOT WAMR_ROOT)
    set(WAMR_ROOT "${CMAKE_CURRENT_LIST_DIR}/../wasm-micro-runtime")
endif()

message(STATUS "Using WAMR ROOT: ${WAMR_ROOT}")

# WAMR Sockets lib
add_library(socket_wasi_ext STATIC ${WAMR_ROOT}/core/iwasm/libraries/lib-socket/src/wasi/wasi_socket_ext.c)
target_include_directories(socket_wasi_ext PUBLIC ${WAMR_ROOT}/core/iwasm/libraries/lib-socket/inc)

# Ocre API
add_library(ocre_api STATIC ocre_api.c)
target_include_directories(ocre_api PUBLIC ${CMAKE_CURRENT_LIST_DIR})
