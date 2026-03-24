cmake_minimum_required(VERSION 3.20.0)

include(${CMAKE_CURRENT_LIST_DIR}/../../ocre.cmake)

project(cat)

add_executable(cat.wasm main.c)
