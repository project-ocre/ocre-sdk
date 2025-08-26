# Ocre SDK and Sample Suite

The Ocre SDK provides a modular C-based API for building portable embedded applications that compile to WebAssembly. It includes both headers and source files, enabling flexible integration across platforms. This repository also contains a wide range of generic and board-specific examples that demonstrate real-world use cases.

All examples are designed to be built using [wasi-sdk](https://github.com/WebAssembly/wasi-sdk) and executed inside the [Ocre Runtime](https://github.com/project-ocre/ocre-runtime), a lightweight WASI-compatible runtime tailored for embedded-style workloads.

## Repository Structure
```
  ├── ocre-api                  # Core SDK: headers + sources
  ├── generic                   # Platform-independent examples
  │   ├── blinky, echo-server, filesystem, etc. 
  │   └── build folders per sample 
  ├── board_specific            # Hardware-targeted examples 
  │   ├── arduino_portenta_h7 
  │   └── b_u585i_iot02a
  ├── wasm-micro-runtime        # External module
```

Each sample includes:
- CMakeLists.txt for build configuration
- A build/ directory (created during compilation)
- Optional subfolders like src/, fs/, or IMU/ depending on the sample

## Getting Started

### Clone the Repository

To get started with the full example suite:

```bash
git clone --recursive https://github.com/project-ocre/getting-started.git
```

Or add the SDK as a submodule to your own project:

```bash
git submodule add https://github.com/project-ocre/ocre-sdk.git
```

Make sure the ocre-api folder is accessible to your build system.

## Building with wasi-sdk
All examples are designed to compile to .wasm using [wasi-sdk](https://github.com/WebAssembly/wasi-sdk).

### Build Instructions
Each sample can be built independently. Here's a generic flow

```bash
cd generic/blinky
mkdir -p build && cd build
cmake ..
cmake --build .
```
This will generate a .wasm binary in the build/ directory.

Repeat the same process for any other sample—whether under generic/ or board_specific/

## Running with Ocre Runtime
