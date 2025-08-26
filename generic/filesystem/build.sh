#!/bin/sh

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
# wasm-objdump -x *.wasm | grep -A1 Import
wasm-objdump -x *.wasm | grep "global\[0\]"
