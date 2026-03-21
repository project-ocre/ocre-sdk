cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../../scripts/wasm32-wasi-pthread.cmake)

project(syslog_webserver)

set(CMAKE_BUILD_TYPE Release)

add_executable(syslog_webserver.wasm
    main.c
    mongoose.c
)

target_compile_options(syslog_webserver.wasm
    PRIVATE
    -Os -Wno-unknown-attributes
    -D_WASI_EMULATED_PTHREAD
)
target_link_options(syslog_webserver.wasm
    PRIVATE
    -z stack-size=8192
)

target_link_libraries(syslog_webserver.wasm
    socket_wasi_ext
    ocre_api
)
