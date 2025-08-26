cmake_minimum_required(VERSION 3.20.0)
set(CMAKE_TOOLCHAIN_FILE /opt/wasi-sdk/share/cmake/wasi-sdk.cmake)
project(ocre_api LANGUAGES C)

add_library(ocre_api STATIC ocre_api.c)
target_include_directories(ocre_api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(ocre_api PRIVATE -O3 -Wall -Wextra -Wno-unused-parameter -Wno-unknown-attributes)
install(TARGETS ocre_api ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
install(FILES ocre_api.h DESTINATION include)