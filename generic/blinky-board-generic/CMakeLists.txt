cmake_minimum_required(VERSION 3.20.0)

include(${CMAKE_CURRENT_LIST_DIR}/../../ocre.cmake)

add_subdirectory(../../ocre-sdk ocre-sdk)

project(blinky-board-generic)

add_executable(blinky-board-generic.wasm main.c)

target_link_libraries(blinky-board-generic.wasm
    PUBLIC
    ocre_api
)
