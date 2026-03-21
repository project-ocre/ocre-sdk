cmake_minimum_required (VERSION 3.20.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../../../../scripts/wasm32-wasi-pthread.cmake)

project(subscriber_temp)

add_executable(subscriber_temp.wasm main.c)

target_link_libraries(subscriber_temp.wasm
    PUBLIC
    ocre_api
)
