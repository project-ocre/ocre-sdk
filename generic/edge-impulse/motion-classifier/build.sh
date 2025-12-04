#!/bin/sh

# Ensure build directory exists
mkdir -p build
cd build

# Run cmake if no makefile
if [ ! -f Makefile ]; then
    cmake ..
fi

# build
make -j
ls -al *.wasm

# If using wasi-sdk
wasm-objdump -x *.wasm | grep -A1 Memory\\[
wasm-objdump -x *.wasm | grep "global\[0\]"

echo "Building standalone classifier OK"
