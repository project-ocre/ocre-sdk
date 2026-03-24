cmake_minimum_required (VERSION 3.20.0)

include(${CMAKE_CURRENT_LIST_DIR}/../../ocre.cmake)

add_subdirectory(../../ocre-sdk ocre-sdk)

project(sensor-rng)

add_executable(sensor-rng.wasm main.c)

target_link_libraries(sensor-rng.wasm
    PUBLIC
    ocre_api
)
