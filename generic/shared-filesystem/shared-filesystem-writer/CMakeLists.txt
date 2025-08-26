cmake_minimum_required (VERSION 3.20.0)

# Set the WASI SDK toolchain file
set(CMAKE_TOOLCHAIN_FILE /opt/wasi-sdk/share/cmake/wasi-sdk-p1.cmake)

set(CMAKE_BUILD_TYPE Release)

set(APPNAME shared-filesystem-writer)
project(${APPNAME})

# When compiling with no stdlib
# add_compile_options(
#     --target=wasm32-wasm-unknown 
#     -nostdlib    
# )

add_compile_options(
    -Os 
    -Wno-unknown-attributes
)

add_link_options(
    -Wl,--strip-all             # Strip all symbol/debug information from the binary
    -Wl,--allow-undefined       # Let the ocre api functions be undefined
    -z stack-size=1024          # Probably larger than needed - optimize
    -Wl,--initial-memory=65536  # Minimum size of linear memory\
    -Wl,--max-memory=65536      # Maximum size of linear memory
)

add_executable(${APPNAME}.wasm main.c)
