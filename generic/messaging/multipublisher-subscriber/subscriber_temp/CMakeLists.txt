cmake_minimum_required (VERSION 3.20.0)

include(${CMAKE_CURRENT_LIST_DIR}/../../../../ocre.cmake)

add_subdirectory(../../../../ocre-sdk ocre-sdk)

project(subscriber_temp)

add_executable(subscriber_temp.wasm main.c)

target_link_libraries(subscriber_temp.wasm
    PUBLIC
    ocre_api
)
