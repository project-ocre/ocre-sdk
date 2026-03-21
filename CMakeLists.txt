cmake_minimum_required (VERSION 3.20.0)

project(ocre-sdk)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

make_directory(${CMAKE_CURRENT_LIST_DIR}/sysroot)

include(ExternalProject)
ExternalProject_Add(wasi-sysroot
  URL https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-32/wasi-sysroot-32.0.tar.gz
  URL_HASH SHA256=f2537f6e5804f7f24e32dd140e9371e9670a9fd2646b6a813ddb896b29954b12
  DOWNLOAD_EXTRACT_TIMESTAMP FALSE
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND cp -r ../wasi-sysroot/. ${CMAKE_CURRENT_LIST_DIR}/sysroot
)

ExternalProject_Add(ocre-sdk
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/ocre-sdk
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  CMAKE_ARGS "-DWAMR_ROOT:STRING=${WAMR_ROOT}"
  DEPENDS wasi-sysroot
)

add_custom_target(sysroot
    DEPENDS
    wasi-sysroot
    ocre-sdk
)

# Containers

make_directory(${CMAKE_CURRENT_BINARY_DIR}/dist)

# Generic Samples

ExternalProject_Add(big-sample
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/big-sample
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp big-sample.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(blinky
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/blinky
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp blinky.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(blinky-board-generic
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/blinky-board-generic
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp blinky-board-generic.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(echo-server
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/echo-server
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp echo-server.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(filesystem
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/filesystem
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp filesystem.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(filesystem-full
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/filesystem-full
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp filesystem-full.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(hello-world
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/hello-world
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp hello-world.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(log_mirror_forwarder
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/log_mirror_forwarder
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp syslog_webserver.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(publisher
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/messaging/publisher
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp publisher.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(subscriber
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/messaging/subscriber
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp subscriber.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(publisher_inside
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/messaging/multipublisher-subscriber/publisher_inside
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp publisher_inside.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(publisher_outside
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/messaging/multipublisher-subscriber/publisher_outside
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp publisher_outside.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(subscriber_temp
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/messaging/multipublisher-subscriber/subscriber_temp
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp subscriber_temp.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(modbus-client
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/modbus-client
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp modbus-client.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(shared-filesystem-reader
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/shared-filesystem/shared-filesystem-reader
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp shared-filesystem-reader.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(shared-filesystem-writer
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/shared-filesystem/shared-filesystem-writer
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp shared-filesystem-writer.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(webserver
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/webserver
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp webserver.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(webserver-complex
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/generic/webserver-complex
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp webserver-complex.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

# Testing

ExternalProject_Add(print_args
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/testing/print_args
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp print_args.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(pthread
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/testing/pthread
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp pthread.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(return0
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/testing/return0
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp return0.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(return1
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/testing/return1
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp return1.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)

ExternalProject_Add(sleep5_return0
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/testing/sleep5_return0
  CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE:BOOL=${CMAKE_VERBOSE_MAKEFILE}"
  INSTALL_COMMAND cp sleep5_return0.wasm ${CMAKE_CURRENT_BINARY_DIR}/dist
  DEPENDS ocre-sdk
)
