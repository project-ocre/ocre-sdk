cmake_minimum_required(VERSION 3.20.0)

include(${CMAKE_CURRENT_LIST_DIR}/../../ocre.cmake)

add_subdirectory(../../ocre-sdk ocre-sdk)

project(echo-server)

add_executable(echo-server.wasm src/main.c)

target_link_libraries(echo-server.wasm
    PUBLIC
    socket_wasi_ext
)
