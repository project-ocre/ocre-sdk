cmake_minimum_required (VERSION 3.20.0)

include(${CMAKE_CURRENT_LIST_DIR}/../../../ocre.cmake)

project(sensor)

add_executable(sensor.wasm main.c)

add_subdirectory(../../../ocre-sdk ocre-sdk)

target_link_libraries(sensor.wasm
    PUBLIC
    ocre_api
)
