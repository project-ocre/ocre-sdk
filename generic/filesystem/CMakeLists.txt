cmake_minimum_required (VERSION 3.20.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../../scripts/wasm32-wasi-pthread.cmake)

project(filesystem)

add_executable(filesystem.wasm main.c)
