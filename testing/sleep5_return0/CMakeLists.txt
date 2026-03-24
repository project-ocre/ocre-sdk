cmake_minimum_required(VERSION 3.20.0)

include(${CMAKE_CURRENT_LIST_DIR}/../../ocre.cmake)

project(sleep5_return0)

add_executable(sleep5_return0.wasm main.c)
