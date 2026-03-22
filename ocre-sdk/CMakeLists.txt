cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../scripts/wasm32-wasi-pthread.cmake)

project(ocre-sdk)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

if (NOT WAMR_ROOT)
    set(WAMR_ROOT "${CMAKE_CURRENT_LIST_DIR}/../wasm-micro-runtime")
endif()

message(STATUS "Using WAMR ROOT: ${WAMR_ROOT}")

# WAMR Sockets lib
add_library(socket_wasi_ext STATIC ${WAMR_ROOT}/core/iwasm/libraries/lib-socket/src/wasi/wasi_socket_ext.c)
target_include_directories(socket_wasi_ext PUBLIC ${WAMR_ROOT}/core/iwasm/libraries/lib-socket/inc)
install(TARGETS socket_wasi_ext DESTINATION ${WASI_SYSROOT}/lib/${triple})
install(FILES ${WAMR_ROOT}/core/iwasm/libraries/lib-socket/inc/wasi_socket_ext.h DESTINATION ${WASI_SYSROOT}/include/${triple}/)

# Ocre API
add_library(ocre_api STATIC ocre_api.c)
install(TARGETS ocre_api DESTINATION ${WASI_SYSROOT}/lib/${triple})
install(FILES ocre_api.h DESTINATION ${WASI_SYSROOT}/include/${triple})
