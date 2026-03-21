cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../scripts/wasm32-wasi-pthread.cmake)

project(testing)

add_executable(print_args.wasm print_args.c)
add_executable(pthread.wasm pthread.c)
add_executable(return0.wasm return0.c)
add_executable(return1.wasm return1.c)
add_executable(sleep_5_return_0.wasm sleep_5_return_0.c)
