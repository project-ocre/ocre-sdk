# Cmake toolchain description file for the Makefile

if(NOT WASI_SYSROOT)
    set(WASI_SYSROOT "${CMAKE_CURRENT_LIST_DIR}/../sysroot")
endif()

set(CMAKE_SYSTEM_NAME WASI)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR wasm32)

set(triple wasm32-wasi-threads)

set(CMAKE_C_FLAGS "-pthread --sysroot=${WASI_SYSROOT}")
set(CMAKE_CXX_FLAGS "-pthread --sysroot=${WASI_SYSROOT}")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--import-memory -Wl,--export-memory -Wl,--strip-all -Wl,--allow-undefined -Wl,--max-memory=4194304")

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_AR llvm-ar)
set(CMAKE_RANLIB llvm-ranlib)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})
set(CMAKE_ASM_COMPILER_TARGET ${triple})

# Don't look in the sysroot for executables to run during the build
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# Only look in the sysroot (not in the host paths) for the rest
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
