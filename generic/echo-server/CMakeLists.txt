cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../../scripts/wasm32-wasi-pthread.cmake)

project(echo-server)

add_executable(echo-server.wasm src/main.c)

target_link_libraries(echo-server.wasm
    PUBLIC
    socket_wasi_ext
)
