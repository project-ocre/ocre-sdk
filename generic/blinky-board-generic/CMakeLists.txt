cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../../scripts/wasm32-wasi-pthread.cmake)

project(blinky-board-generic)

add_executable(blinky-board-generic.wasm main.c)

target_link_libraries(blinky-board-generic.wasm
    PUBLIC
    ocre_api
)
