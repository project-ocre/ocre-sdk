cmake_minimum_required(VERSION 3.20.0)

# Set the WASI SDK toolchain file
set(CMAKE_TOOLCHAIN_FILE /opt/wasi-sdk/share/cmake/wasi-sdk-p1.cmake)

# Set build type to Release
set(CMAKE_BUILD_TYPE Release)

# Project name
set(APPNAME edge-impulse-standalone)
project(${APPNAME})

# Set compilation flags
add_compile_options(
    # --target=wasm32-wasi
    -Os                               # 155k size
    # -O3                               # 188k size
    -DNDEBUG
    -Wall
    -DEI_PORTING_POSIX=1              # Force POSIX port for Wasm
    -DUSE_CMSIS_NN=OFF                # Wasm does not support CMSIS instructions
    -DEI_LOG_LEVEL=4                  # Debug logging
    -DTF_LITE_DISABLE_X86_NEON=1      # Disable x86 NEON optimizations - prob not needed?
    -DEIDSP_SIGNAL_C_FN_POINTER=1     # Using the C library - required parameter
    -DEI_C_LINKAGE=1                  # Using the C library - required parameter
    -fno-exceptions                   # Exceptions not supported in Wasm
    -Wno-strict-aliasing
    -Wno-unknown-attributes
    -I${CMAKE_SOURCE_DIR}
)

# Set C++ standard for C++ files
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set linker flags
add_link_options(
    -Wl,--strip-all
    -Wl,--allow-undefined
    -lm
)

# Define source files (mirroring Makefile)
set(CSOURCES
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/tensorflow/lite/c/common.c
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/Source/BasicMathFunctions/*.c
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/Source/FastMathFunctions/*.c
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/Source/StatisticsFunctions/*.c
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/Source/TransformFunctions/*fft*.c
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/Source/CommonTables/*.c
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/Source/TransformFunctions/*bit*.c
)

set(CXXSOURCES
    ${CMAKE_SOURCE_DIR}/tflite-model/*.cpp
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/dsp/kissfft/*.cpp
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/dsp/dct/*.cpp
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/dsp/memory.cpp
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/classifier/*.cpp
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/porting/posix/*.c*
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/porting/mingw32/*.c*
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/tensorflow/lite/kernels/*.cc
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/tensorflow/lite/kernels/internal/*.cc
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/tensorflow/lite/micro/kernels/*.cc
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/tensorflow/lite/micro/*.cc
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/tensorflow/lite/micro/memory_planner/*.cc
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/tensorflow/lite/core/api/*.cc
)

set(APP_CSOURCES
    ${CMAKE_SOURCE_DIR}/source/main.c
)

# Optional CMSIS-NN sources
option(USE_CMSIS_NN "Enable CMSIS-NN optimizations" OFF)
if(USE_CMSIS_NN)
    add_compile_options(
        -DEI_CLASSIFIER_TFLITE_ENABLE_CMSIS_NN=1
        -D__ARM_FEATURE_DSP=1
        -D__GNUC_PYTHON__=1
        -I${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/NN/Include/
        -I${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/PrivateInclude/
    )
    list(APPEND CSOURCES
        ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/NN/Source/ActivationFunctions/*.c
        ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/*.c
        ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/NN/Source/ConcatenationFunctions/*.c
        ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/NN/Source/ConvolutionFunctions/*.c
        ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/NN/Source/FullyConnectedFunctions/*.c
        ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/NN/Source/NNSupportFunctions/*.c
        ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/NN/Source/PoolingFunctions/*.c
        ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/NN/Source/ReshapeFunctions/*.c
        ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/NN/Source/SoftmaxFunctions/*.c
    )
endif()

# Glob source files
file(GLOB CSOURCE_FILES ${CSOURCES})
file(GLOB CXXSOURCE_FILES ${CXXSOURCES})
file(GLOB APP_CSOURCE_FILES ${APP_CSOURCES})

# Create executable
add_executable(${APPNAME}.wasm ${CSOURCE_FILES} ${CXXSOURCE_FILES} ${APP_CSOURCE_FILES})

# Link libraries (math library included via add_link_options)
target_link_libraries(${APPNAME}.wasm PRIVATE m)