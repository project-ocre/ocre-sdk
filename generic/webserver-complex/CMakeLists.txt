cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../../scripts/wasm32-wasi-pthread.cmake)

project(webserver-complex)

set(CMAKE_BUILD_TYPE Release)

add_executable(webserver-complex.wasm
    main.c
    mongoose.c
)

target_compile_options(webserver-complex.wasm
    PRIVATE
    -Os -Wno-unknown-attributes
    -D_WASI_EMULATED_PTHREAD
)
target_link_options(webserver-complex.wasm
    PRIVATE
    -z stack-size=8192
    -Wl,--initial-memory=65536  # Minimum size of linear memory
    -Wl,--max-memory=65536      # Maximum size of linear memory
)

target_link_libraries(webserver-complex.wasm
    socket_wasi_ext
    ocre_api
)
