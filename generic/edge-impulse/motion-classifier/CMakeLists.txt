cmake_minimum_required(VERSION 3.20.0)

# Project name / languages
set(APPNAME edge-impulse-standalone)
project(${APPNAME} C CXX)

# ------------------------------------------------------------
# Options
# ------------------------------------------------------------

# When ON, build a WASI/WASM binary. When OFF, build a native binary.
option(BUILD_WASM "Build WASM/WASI binary (requires WASI SDK toolchain)" OFF)

# Optional CMSIS-NN sources (same as your original file)
option(USE_CMSIS_NN "Enable CMSIS-NN optimizations" OFF)

# ------------------------------------------------------------
# Common settings
# ------------------------------------------------------------

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Common preprocessor definitions shared between native & WASM
set(COMMON_DEFINES
    EI_PORTING_POSIX=1              # Force POSIX port for WASI compatibility
    USE_CMSIS_NN=OFF                # Default OFF since WASI does not support
    EI_LOG_LEVEL=4
    TF_LITE_DISABLE_X86_NEON=1
    EIDSP_SIGNAL_C_FN_POINTER=1
    EI_C_LINKAGE=1
)

# Source lists (same structure as your original CMake)
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

if(USE_CMSIS_NN)
    list(APPEND COMMON_DEFINES
        EI_CLASSIFIER_TFLITE_ENABLE_CMSIS_NN=1
        __ARM_FEATURE_DSP=1
        __GNUC_PYTHON__=1
    )
    include_directories(
        ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/NN/Include/
        ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/PrivateInclude/
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

# Glob the sources
file(GLOB CSOURCE_FILES ${CSOURCES})
file(GLOB CXXSOURCE_FILES ${CXXSOURCES})
file(GLOB APP_CSOURCE_FILES ${APP_CSOURCES})

set(ALL_SOURCES
    ${CSOURCE_FILES}
    ${CXXSOURCE_FILES}
    ${APP_CSOURCE_FILES}
)

# ------------------------------------------------------------
# WASM / Native targets
# ------------------------------------------------------------

if(BUILD_WASM)

    add_executable(${APPNAME}.wasm ${ALL_SOURCES})

    target_compile_definitions(${APPNAME}.wasm PRIVATE ${COMMON_DEFINES})

    target_compile_options(${APPNAME}.wasm PRIVATE
        -Os
        -DNDEBUG
        -Wall
        -fno-exceptions
        -Wno-strict-aliasing
        -Wno-unknown-attributes
        -I${CMAKE_SOURCE_DIR}
    )

    # Linker options specific to WASI / WASM
    target_link_options(${APPNAME}.wasm PRIVATE
        -Wl,--strip-all
        -Wl,--allow-undefined
        -lm
    )

    target_link_libraries(${APPNAME}.wasm PRIVATE m)

else()
    # Native build
    add_executable(${APPNAME} ${ALL_SOURCES})

    target_compile_definitions(${APPNAME} PRIVATE ${COMMON_DEFINES})

    target_compile_options(${APPNAME} PRIVATE
        -Os
        -DNDEBUG
        -Wall
        -Wno-strict-aliasing
        -I${CMAKE_SOURCE_DIR}
    )

    target_link_options(${APPNAME} PRIVATE
        -Wl,--strip-all    
        -lm
    )

    target_link_libraries(${APPNAME} PRIVATE m)

endif()
