cmake_minimum_required(VERSION 3.20.0)

include(${CMAKE_CURRENT_LIST_DIR}/../../ocre.cmake)

project(big-sample)

add_executable(big-sample.wasm main.c)
