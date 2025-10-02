#!/bin/sh
# /opt/wasi-sdk/bin/clang -I . -o modbus-slave.wasm ./wasi_socket_ext.c ./mongoose.c ./main.c -Wl,--strip-all
# ls -al modbus-slave.wasm

# Ensure build directory exists
mkdir -p build
cd build

# Run cmake if no makefile
if [ ! -f Makefile ]; then
    cmake ..
fi

# build
make
ls -al *.wasm

# If using wasi-sdk-pthread
# wasm-objdump -x *.wasm | grep -A1 Import

# If using wasi-sdk
wasm-objdump -x *.wasm | grep -A1 Memory
wasm-objdump -x *.wasm | grep "global\[0\]"