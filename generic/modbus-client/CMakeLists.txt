cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../../scripts/wasm32-wasi-pthread.cmake)

project(modbus-client)

set(CMAKE_BUILD_TYPE Release)

add_executable(modbus-client.wasm main.c mongoose.c)

target_compile_options(modbus-client.wasm
    PRIVATE
    -Os -Wno-unknown-attributes
)

target_link_options(modbus-client.wasm
    PRIVATE
    -z stack-size=16384
    -Wl,--initial-memory=65536  # Minimum size of linear memory
    -Wl,--max-memory=65536      # Maximum size of linear memory
)

target_link_libraries(modbus-client.wasm
    socket_wasi_ext
    ocre_api
)
