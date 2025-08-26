cmake_minimum_required (VERSION 3.20.0)

# Set the WASI SDK toolchain file
set(CMAKE_TOOLCHAIN_FILE /opt/wasi-sdk/share/cmake/wasi-sdk-p1.cmake)

set(CMAKE_BUILD_TYPE Release)

set(APPNAME modbus-client)
project(${APPNAME})

# Include the WAMR socket library CMake file (adjust path as needed)
include(../../wasm-micro-runtime/core/iwasm/libraries/lib-socket/lib_socket_wasi.cmake)

add_compile_options(
#    --target=wasm32-wasm-unknown # Target WebAssembly
    -Os -Wno-unknown-attributes
)

# Set compilation flags
add_link_options(
    -Wl,--strip-all             # Strip all symbol/debug information from the binary
    -Wl,--allow-undefined       # Let the ocre api functions be undefined
    -z stack-size=16384
    -Wl,--initial-memory=65536  # Minimum size of linear memory\
    -Wl,--max-memory=65536      # Maximum size of linear memory
)

# Add the ocre-api as a subdirectory and build it
set(OCRE_API_PATH "../../ocre-api")
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${OCRE_API_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "ocre-api not found at ${OCRE_API_PATH}. Please ensure the ocre-api directory exists and contains CMakeLists.txt")
endif()
add_subdirectory(${OCRE_API_PATH} ocre-api-build)

add_executable(${APPNAME}.wasm main.c mongoose.c)

# Include + link the Ocre API and WAMR socket library
target_include_directories(${APPNAME}.wasm PRIVATE
    ../../wasm-micro-runtime/core/iwasm/libraries/lib-socket
    ocre_api
)
target_link_libraries(${APPNAME}.wasm socket_wasi_ext ocre_api)
