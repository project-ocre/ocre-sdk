cmake_minimum_required (VERSION 3.20.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../../scripts/wasm32-wasi-pthread.cmake)

project(sensor-rng)

add_executable(sensor-rng.wasm main.c)

target_link_libraries(sensor-rng.wasm
    PUBLIC
    ocre_api
)
