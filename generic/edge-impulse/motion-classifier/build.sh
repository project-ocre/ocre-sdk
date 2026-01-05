#!/bin/sh
set -e

# Usage:
#   ./build.sh native   # build only native binary
#   ./build.sh wasm     # build only WASM/WASI module
#   ./build.sh both     # build both (default)
#
MODE="${1:-both}"

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"

build_native() {
    echo "=== Building native binary ==="
    BUILD_DIR="${ROOT_DIR}/build-native"
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"

    if [ ! -f CMakeCache.txt ]; then
        cmake .. \
          -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_WASM=OFF
    fi

    cmake --build . -- -j

    echo
    echo "Native build outputs:"
    ls -al "${BUILD_DIR}"
    echo "=== Native build done ==="
    echo
}

build_wasm() {
    echo "=== Building WASM/WASI module ==="
    BUILD_DIR="${ROOT_DIR}/build-wasm"
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"

    cmake .. \
        -DBUILD_WASM=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/opt/wasi-sdk/share/cmake/wasi-sdk-p1.cmake

    cmake --build . -- -j

    echo
    echo "WASM build outputs:"
    ls -al *.wasm

    # inspect the module a bit
    if command -v wasm-objdump >/dev/null 2>&1; then
        wasm-objdump -x *.wasm | grep -A1 'Memory\[' || true
        wasm-objdump -x *.wasm | grep 'global\[0\]' || true
    else
        echo "wasm-objdump not found in PATH; skipping WASM inspection"
    fi

    echo "Building standalone classifier (WASM) OK"
    echo "=== WASM build done ==="
    echo
}

clean() {
    echo "=== Cleaning build directories ==="
    rm -rf "${ROOT_DIR}/build-native" "${ROOT_DIR}/build-wasm"
    echo "=== Clean done ==="
    echo
}

build_atym() {
    echo "=== Building for ATYM ==="
    DATA_CONT_DIR="${ROOT_DIR}/container-data"
    
    cd "${DATA_CONT_DIR}"
    atym build
    atym push ei-data

    echo

    CLASS_CONT_DIR="${ROOT_DIR}/container-classifier"
    cd "${CLASS_CONT_DIR}"
    atym build
    atym push ei-classifier -a aot.yaml

    echo
    echo "=== ATYM build done ==="
    echo
}

case "${MODE}" in
    clean)
        clean
        ;;
    native)
        build_native
        ;;
    wasm)
        build_wasm
        ;;
    atym)
        build_wasm
        build_atym
        ;;
    both)
        build_native
        build_wasm
        ;;
    *)
        echo "Unknown mode: ${MODE}"
        echo "Usage: $0 [clean|native|wasm|both]"
        exit 1
        ;;
esac
