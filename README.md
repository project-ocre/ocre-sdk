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

> **Note:** By default, we expect the WASI SDK toolchain file to be located at:
>
> `/opt/wasi-sdk/share/cmake/wasi-sdk.cmake`
>
> This is typically set in each sample's `CMakeLists.txt`. You can modify this path as needed for your environment.

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
All compiled .wasm samples are compatible with the [Ocre Runtime](https://github.com/project-ocre/ocre-runtime), which provides a lightweight execution environment for WASI modules.

Example:
```bash
app build/hello-world.wasm
```

## Example Categories
### Generic Samples
- blinky
- hello-world
- echo-server
- filesystem, filesystem-full, shared-filesystem
- webserver
- messaging: publisher, subscriber, multipublisher-subscriber
- modbus-client
- sensor-rng
### Board-Specific Samples
- arduino_portenta_h7: blinky-h7
- b_u585i_iot02a: sensor, sensor-IMU, modbus-server, blinky-xmas, blinky-u585, blinky-button
These demonstrate hardware-specific integrations while still leveraging the common ocre-api.
## SDK Highlights
- Header and source-based SDK (ocre-api)
- Modular CMake-based build system
- Runtime execution via Ocre Runtime
- Extensible for new boards and applications

## Contributing
Want to add a new board or example? Fork the repo, create your sample under generic/ or board_specific/, and submit a pull request. Contributions are welcome
