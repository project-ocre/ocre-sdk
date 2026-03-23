cmake_minimum_required (VERSION 3.20.0)

include(${CMAKE_CURRENT_LIST_DIR}/../../../ocre.cmake)

project(modbus-server)

add_executable(modbus-server.wasm main.c mongoose.c)

add_subdirectory(../../../ocre-sdk ocre-sdk)

target_link_libraries(modbus-server.wasm
    PUBLIC
    ocre_api
    socket_wasi_ext
)
