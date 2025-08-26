cmake_minimum_required(VERSION 3.20.0)
set(CMAKE_TOOLCHAIN_FILE /opt/wasi-sdk/share/cmake/wasi-sdk.cmake)
set(APPNAME blinky-xmas)
project(${APPNAME} LANGUAGES C)

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--strip-all -Wl,--allow-undefined")
add_compile_options(
    -O3
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unknown-attributes
)

# Add the ocre-api as a subdirectory and build it
set(OCRE_API_PATH "../../../ocre-api")
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${OCRE_API_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "ocre-api not found at ${OCRE_API_PATH}. Please ensure the ocre-api directory exists and contains CMakeLists.txt")
endif()

add_subdirectory(${OCRE_API_PATH} ocre-api-build)

add_executable(${APPNAME}.wasm main.c)
target_include_directories(${APPNAME}.wasm PRIVATE ocre_api)
target_link_libraries(${APPNAME}.wasm PRIVATE ocre_api)
