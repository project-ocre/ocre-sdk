cmake_minimum_required (VERSION 3.20.0)

include(${CMAKE_CURRENT_LIST_DIR}/../../../ocre.cmake)

project(blinky-u585)

add_executable(blinky-u585.wasm main.c)

add_subdirectory(../../../ocre-sdk ocre-sdk)

target_link_libraries(blinky-u585.wasm
    PUBLIC
    ocre_api
)
