set(CMAKE_TOOLCHAIN_FILE /opt/wasi-sdk/share/cmake/wasi-sdk.cmake)

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--import-memory -Wl,--export-memory -Wl,--strip-all -Wl,--allow-undefined -Wl,--max-memory=4194304")
