cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../../scripts/wasm32-wasi-pthread.cmake)

project(sleep5_return0)

add_executable(sleep5_return0.wasm main.c)
