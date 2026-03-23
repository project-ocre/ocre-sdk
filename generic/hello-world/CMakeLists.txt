cmake_minimum_required(VERSION 3.20.0)

include(${CMAKE_CURRENT_LIST_DIR}/../../ocre.cmake)

project(hello-world)

add_executable(hello-world.wasm main.c)
