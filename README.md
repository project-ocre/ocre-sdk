# Ocre SDK and Sample Suite

The Ocre SDK provides a modular C-based API for building portable embedded applications that compile to WebAssembly. It includes both headers and source files, enabling flexible integration across platforms. This repository also contains a wide range of generic and board-specific examples that demonstrate real-world use cases.

All examples are designed to be executed inside the [Ocre Runtime](https://github.com/project-ocre/ocre-runtime), a lightweight WASI-compatible runtime tailored for embedded-style workloads.

## Repository Structure

```
  ├── ocre-api                  # Core SDK: headers + sources
  ├── generic                   # Platform-independent examples
  │   ├── blinky, echo-server, filesystem, etc.
  │   └── build folders per sample
  ├── board_specific            # Hardware-targeted examples
  │   ├── arduino_portenta_h7
  │   └── b_u585i_iot02a
  ├── testing                   # Testing and potentially faulty images.
  │   └── return0, return1, pthread, etc.
  ├── wasm-micro-runtime        # External module
```

## Getting Started

### System requirements

To build the samples, you need LLVM, LLD, Clang 12+, wasm32 runtime libraries. You also need cmake, make and **\***. Install all on an Ubuntu 22 with:

```sh
sudo apt install \
    cmake \
    clang \
    libclang-rt-dev-wasm32 \
    lld \
    llvm
```

Alternatively, you can use the Ocre devcontainer. Check Ocre documentation for more information.

### Clone the Repository

To get started with the full example suite:

```bash
git clone --recursive https://github.com/project-ocre/ocre-sdk.git
```

Or add the SDK as a submodule to your own project:

```bash
git submodule add https://github.com/project-ocre/ocre-sdk.git
```

## Set up Sysroot

The default sysroot can be set up automatically. From the root of this repository:

```sh
mkdir build
cd build
cmake ..
make sysroot
```

The `sysroot` Make target will download the wasi-sysroot, then compile `lib_socket_ext` and `ocre_api`, and populate them into the `sysroot` directory on the root of this repository.

It is also possible to specify a different WAMR root, used for building `lib_socket_ext`, through the `WAMR_ROOT` CMake variable, if you do not want to use the included git submodule:

```sh
cmake .. -DWAMR_ROOT=~/ocre/wasm-micro-runtime
```

## Build Samples

### From the ocre-sdk build directory

Alternatively, the `all` Make target will build most of the generic samples and testing containers:

```sh
make
```

You can also build a specific sample, for example:

```sh
make hello-world
```

The built files are stord in the `dist` directory inside the current build directory:

```sh
ls dist
```

### From each sample's directory

Assumming, you already have the sysroot installed into the `sysroot` directory, it is possible to build each sample from their own directory, for example:

```sh
cd generic/hello-world
mkdir build
cd build
cmake ..
make
```

The built file is stored in the current build directory.

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
