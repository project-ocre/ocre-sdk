cmake_minimum_required (VERSION 3.20.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../../scripts/wasm32-wasi-pthread.cmake)

project(filesystem-full)

set(CMAKE_BUILD_TYPE Release)

add_executable(filesystem-full.wasm main.c)

target_compile_options(filesystem-full.wasm
    PRIVATE
    -Os
    -Wno-unknown-attributes
)

target_link_options(filesystem-full.wasm
    PRIVATE
    -z stack-size=1024          # Probably larger than needed
    -Wl,--initial-memory=65536  # Minimum size of linear memory
    -Wl,--max-memory=65536      # Maximum size of linear memory
)
