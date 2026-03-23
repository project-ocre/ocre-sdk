cmake_minimum_required (VERSION 3.20.0)

include(${CMAKE_CURRENT_LIST_DIR}/../../../ocre.cmake)

add_subdirectory(../../../ocre-sdk ocre-sdk)

project(subscriber)

add_executable(subscriber.wasm main.c)

target_link_libraries(subscriber.wasm
    PUBLIC
    ocre_api
)
