cmake_minimum_required (VERSION 3.20.0)

include(${CMAKE_CURRENT_LIST_DIR}/../../../ocre.cmake)

add_subdirectory(../../../ocre-sdk ocre-sdk)

project(shared-filesystem-reader)

add_executable(shared-filesystem-reader.wasm main.c)
