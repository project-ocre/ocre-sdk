cmake_minimum_required (VERSION 3.20.0)

include(${CMAKE_CURRENT_LIST_DIR}/../../ocre.cmake)

project(blinky)

add_executable(blinky.wasm main.c)

add_subdirectory(../../ocre-sdk ocre-sdk)

target_link_libraries(blinky.wasm
    PUBLIC
    ocre_api
)
