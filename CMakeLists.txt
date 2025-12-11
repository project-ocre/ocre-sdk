cmake_minimum_required (VERSION 3.20.0)

# Set the WASI SDK toolchain file
set(CMAKE_TOOLCHAIN_FILE /opt/wasi-sdk/share/cmake/wasi-sdk-pthread.cmake)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

project(OcreSdkSamples)

# Set the linker flags
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--strip-all -Wl,--allow-undefined -Wl,--max-memory=1048576")

# Set compilation flags
add_compile_options(
    -O0 -Wno-unknown-attributes
    -O3
    -Wall
    -Wextra
    -Wno-unused-parameter
)

if(NOT WAMR_ROOT_DIR)
    set(WAMR_ROOT_DIR "wasm-micro-runtime")
endif()

include(${WAMR_ROOT_DIR}/core/iwasm/libraries/lib-socket/lib_socket_wasi.cmake)

add_subdirectory(ocre-sdk)

# Create the executable targets
add_executable("hello-world.wasm" generic/hello-world/main.c)

add_executable("blinky.wasm" generic/blinky/main.c)

# Include + link the Ocre API and WAMR socket library
target_include_directories("blinky.wasm" PRIVATE
    ocre_api
)

target_link_libraries("blinky.wasm" ocre_api)

add_executable("publisher.wasm" generic/messaging/publisher/main.c)

target_include_directories("publisher.wasm" PRIVATE
    ocre_api
)

target_link_libraries("publisher.wasm" ocre_api)

add_executable("subscriber.wasm" generic/messaging/subscriber/main.c)

target_include_directories("subscriber.wasm" PRIVATE
    ocre_api
)

target_link_libraries("subscriber.wasm" ocre_api)

add_executable("filesystem.wasm" generic/filesystem/main.c)

target_include_directories("filesystem.wasm" PRIVATE
    ocre_api
)

target_link_libraries("filesystem.wasm" ocre_api)

add_executable("filesystem-full.wasm" generic/filesystem-full/main.c)

target_include_directories("filesystem-full.wasm" PRIVATE
    ocre_api
)

target_link_libraries("filesystem-full.wasm" ocre_api)

add_executable("webserver.wasm" generic/webserver/main.c generic/webserver/mongoose.c)

target_include_directories("webserver.wasm" PRIVATE
    ${WAMR_ROOT_DIR}/core/iwasm/libraries/lib-socket
    ocre_api
)

target_link_libraries("webserver.wasm" socket_wasi_ext ocre_api)

add_executable("webserver-complex.wasm" generic/webserver-complex/main.c generic/webserver-complex/mongoose.c)

target_include_directories("webserver-complex.wasm" PRIVATE
    ${WAMR_ROOT_DIR}/core/iwasm/libraries/lib-socket
    ocre_api
)

target_link_libraries("webserver-complex.wasm" socket_wasi_ext ocre_api)

add_executable("return0.wasm" testing/return0.c)
add_executable("return1.wasm" testing/return1.c)
add_executable("sleep_5_return_0.wasm" testing/sleep_5_return_0.c)
add_executable("pthread.wasm" testing/pthread.c)
