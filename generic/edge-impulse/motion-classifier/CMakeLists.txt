cmake_minimum_required(VERSION 3.20.0)

# Project name / languages
set(APPNAME edge-impulse-standalone)
project(${APPNAME} C CXX)

# ------------------------------------------------------------
# Options
# ------------------------------------------------------------

option(BUILD_WASM "Build WASM/WASI binary (requires WASI SDK toolchain)" OFF)
option(USE_CMSIS_NN "Enable CMSIS-NN optimizations" OFF)

# ------------------------------------------------------------
# Common settings / defines
# ------------------------------------------------------------

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the ocre-sdk as a subdirectory and build it
set(OCRE_SDK_PATH "../../../ocre-sdk")
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${OCRE_SDK_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "ocre-sdk not found at ${OCRE_SDK_PATH}. Please ensure the ocre-sdk directory exists and contains CMakeLists.txt")
endif()

add_subdirectory(${OCRE_SDK_PATH} ocre-sdk-build)

set(COMMON_DEFINES
    EI_PORTING_POSIX=1
    USE_CMSIS_NN=OFF
    # EI_LOG_LEVEL=0
    EI_LOG_LEVEL=4
    TF_LITE_DISABLE_X86_NEON=1
    EIDSP_SIGNAL_C_FN_POINTER=1
    EI_C_LINKAGE=1
)

# Root include; adjust if you want more target-local includes instead
include_directories(
    ${CMAKE_SOURCE_DIR}
    ocre_api
)

# ------------------------------------------------------------
# Edge Impulse / TFLM sources (classifier only)
# ------------------------------------------------------------

set(EI_CSOURCES
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/tensorflow/lite/c/common.c
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/Source/BasicMathFunctions/*.c
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/Source/FastMathFunctions/*.c
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/Source/StatisticsFunctions/*.c
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/Source/TransformFunctions/*fft*.c
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/Source/CommonTables/*.c
    ${CMAKE_SOURCE_DIR}/edge-impulse-sdk/CMSIS/DSP/Source/TransformFunctions/*bit*.c
)

set(EI_CXXSOURCES
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

    list(APPEND EI_CSOURCES
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

file(GLOB EI_CSOURCE_FILES  ${EI_CSOURCES})
file(GLOB EI_CXXSOURCE_FILES ${EI_CXXSOURCES})

# ------------------------------------------------------------
# App sources
# ------------------------------------------------------------

# Classifier container main
set(CLASSIFIER_APP_CSOURCES
    ${CMAKE_SOURCE_DIR}/container-classifier/main.c
)

file(GLOB CLASSIFIER_APP_FILES ${CLASSIFIER_APP_CSOURCES})

set(CLASSIFIER_SOURCES
    ${EI_CSOURCE_FILES}
    ${EI_CXXSOURCE_FILES}
    ${CLASSIFIER_APP_FILES}
)

# Data container main + QCBOR only
set(DATA_APP_CSOURCES
    ${CMAKE_SOURCE_DIR}/container-data/main.c
    ${CMAKE_SOURCE_DIR}/container-data/ei_cbor_decoder.c
)
file(GLOB DATA_APP_FILES ${DATA_APP_CSOURCES})

file(GLOB QCBOR_SOURCES
    ${CMAKE_SOURCE_DIR}/QCBOR/src/*.c
)
include_directories(
    ${CMAKE_SOURCE_DIR}/QCBOR/inc
)

set(DATA_SOURCES
    ${DATA_APP_FILES}
    ${QCBOR_SOURCES}
)

# Data container main + QCBOR only
set(DATA_TEST_APP_CSOURCES
    ${CMAKE_SOURCE_DIR}/container-data/test.c
    ${CMAKE_SOURCE_DIR}/container-data/ei_cbor_decoder.c
)
file(GLOB DATA_TEST_APP_FILES ${DATA_TEST_APP_CSOURCES})

set(DATA_TEST_SOURCES
    ${DATA_TEST_APP_FILES}
    ${QCBOR_SOURCES}
)

# ------------------------------------------------------------
# Asset embedder container (CBOR files -> headers -> extractor)
# ------------------------------------------------------------

set(EMBED_CBOR_DIR "${CMAKE_SOURCE_DIR}/data/testing"
    CACHE PATH "Directory containing CBOR files to embed (only *.cbor* files are embedded)")

set(ASSET_OUTPUT_ROOT_DEFAULT "/testing"
    CACHE STRING "Default extraction root inside the runtime filesystem")

set(ASSETS_APP_CSOURCES
    ${CMAKE_SOURCE_DIR}/container-assets/main.c
)
set(ASSETS_APP_FILES ${ASSETS_APP_CSOURCES})

# Inputs (so changes trigger regeneration)
file(GLOB EMBED_CBOR_INPUTS "${EMBED_CBOR_DIR}/*.cbor*")
if(NOT EMBED_CBOR_INPUTS)
    message(FATAL_ERROR "No *.cbor* files found in EMBED_CBOR_DIR='${EMBED_CBOR_DIR}'")
endif()

list(GET EMBED_CBOR_INPUTS 0 first_abs)
message(STATUS "First embedded CBOR: ${first_abs}")

# Output dir for generated headers
set(EMBED_GEN_DIR "${CMAKE_BINARY_DIR}/embedded")
file(MAKE_DIRECTORY "${EMBED_GEN_DIR}")

# One output “stamp” file for the generator step
set(EMBED_STAMP "${EMBED_GEN_DIR}/embedded_assets.stamp")

add_custom_command(
    OUTPUT "${EMBED_STAMP}"
    COMMAND python3
        "${CMAKE_SOURCE_DIR}/container-assets/gen_embedded_assets.py"
        --in-dir "${EMBED_CBOR_DIR}"
        --out-dir "${EMBED_GEN_DIR}"
        --pattern "*.cbor*"
    COMMAND ${CMAKE_COMMAND} -E touch "${EMBED_STAMP}"
    DEPENDS ${EMBED_CBOR_INPUTS} "${CMAKE_SOURCE_DIR}/container-assets/gen_embedded_assets.py"
    VERBATIM
)

add_custom_target(assets_embed_headers DEPENDS "${EMBED_STAMP}")

# ------------------------------------------------------------
# Targets
# ------------------------------------------------------------

if(BUILD_WASM)
    set(CLASSIFIER_TARGET ${APPNAME}-classifier.wasm)
    set(DATA_TARGET       ${APPNAME}-data.wasm)
    set(DATA_TEST_TARGET  ${APPNAME}-data-test.wasm)
    set(ASSETS_TARGET     ${APPNAME}-assets.wasm)

    add_executable(${CLASSIFIER_TARGET} ${CLASSIFIER_SOURCES})
    add_executable(${DATA_TARGET}       ${DATA_SOURCES})
    add_executable(${DATA_TEST_TARGET}  ${DATA_TEST_SOURCES})
    add_executable(${ASSETS_TARGET} ${ASSETS_APP_FILES})
    add_dependencies(${ASSETS_TARGET} assets_embed_headers)

    # Common defines to both
    target_compile_definitions(${CLASSIFIER_TARGET} PRIVATE ${COMMON_DEFINES})
    target_compile_definitions(${DATA_TARGET}       PRIVATE ${COMMON_DEFINES})
    target_compile_definitions(${DATA_TEST_TARGET}  PRIVATE ${COMMON_DEFINES})
    target_compile_definitions(${ASSETS_TARGET} PRIVATE
        ${COMMON_DEFINES}
        ASSET_OUTPUT_ROOT_DEFAULT="${ASSET_OUTPUT_ROOT_DEFAULT}"
    )

    # QCBOR float clean-up for data container
    target_compile_definitions(${DATA_TARGET} PRIVATE
        QCBOR_DISABLE_FLOAT_HW_USE
    )

    target_compile_definitions(${DATA_TEST_TARGET} PRIVATE
        QCBOR_DISABLE_FLOAT_HW_USE
    )

    target_include_directories(${ASSETS_TARGET} PRIVATE
        ${EMBED_GEN_DIR}
    )

    foreach(tgt ${CLASSIFIER_TARGET} ${DATA_TARGET} ${DATA_TEST_TARGET} ${ASSETS_TARGET})
        target_compile_options(${tgt} PRIVATE
            -Os
            -DNDEBUG
            -Wall
            -fno-exceptions
            -Wno-strict-aliasing
            -Wno-unknown-attributes
        )

        target_link_options(${tgt} PRIVATE
            -Wl,--strip-all
            -Wl,--allow-undefined
            -lm
        )

        target_link_libraries(${tgt} PRIVATE m ocre_api)
    endforeach()

else()
    set(CLASSIFIER_TARGET ${APPNAME}-classifier)
    set(DATA_TARGET       ${APPNAME}-data)

    add_executable(${CLASSIFIER_TARGET} ${CLASSIFIER_SOURCES})
    add_executable(${DATA_TARGET}       ${DATA_SOURCES})

    target_compile_definitions(${CLASSIFIER_TARGET} PRIVATE ${COMMON_DEFINES})
    target_compile_definitions(${DATA_TARGET}       PRIVATE ${COMMON_DEFINES})
    target_compile_definitions(${DATA_TARGET} PRIVATE
        QCBOR_DISABLE_FLOAT_HW_USE
    )

    foreach(tgt ${CLASSIFIER_TARGET} ${DATA_TARGET})
        target_compile_options(${tgt} PRIVATE
            -Os
            -DNDEBUG
            -Wall
            -Wno-strict-aliasing
        )

        target_link_options(${tgt} PRIVATE
            -Wl,--strip-all
            -lm
        )

        target_link_libraries(${tgt} PRIVATE m ocre_api)
    endforeach()

endif()
