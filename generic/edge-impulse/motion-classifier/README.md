# Edge Impulse Motion Classifier - Atym Container Example

This example demonstrates a closed-loop inference system using Edge Impulse's motion classifier model on the Atym Ocre container platform. The system runs on both Linux and Zephyr targets and is organized as three independent containerized applications that communicate via the Ocre messaging bus.

## Overview

This project contains:

- **Classifier Container**: Receives raw sensor samples, runs the Edge Impulse motion classifier, and publishes predictions
- **Data Container**: Publishes labeled test samples from the dataset, collects prediction results, and reports overall accuracy
- **Assets Container**: Embeds the test dataset CBOR files and extracts them to the filesystem at runtime

The containers communicate through an internal messaging bus, allowing the data publisher to validate predictions in a closed-loop fashion for testing and validation purposes.

## Quick Start

### Prerequisites

To replicate this example with a different model:
1. Clone or create a motion classification project on [Edge Impulse](https://www.edgeimpulse.com/)
2. Export the trained impulse as a **C Library** (not C++ Standalone)
3. Extract the exported library to this directory (replacing the existing `edge-impulse-sdk` folder)
4. Obtain the test dataset CBOR files from your Edge Impulse project's data collection section
5. Place all CBOR files in the `data/testing/` directory

### Building

Before building for Atym, ensure you're logged in:
```bash
atym login
```

Then build and push containers:
```bash
./build.sh atym
```

This will:
1. Build all three containers
2. Automatically push them to your Atym registry as `ei-assets`, `ei-data`, and `ei-classifier`

## Deployment

### Deployment Order (Critical)

The containers must be deployed in this specific order:

1. **Assets Container** - Extracts the test dataset to `/testing` on the device filesystem
2. **Data Container** - Publishes sample windows and collects validation results
3. **Classifier Container** - Responds to incoming samples with predictions

Why this order matters:
- The data container needs the dataset files (extracted by assets)
- The classifier must be listening before the data container starts publishing
- On Zephyr, deploy assets first to ensure filesystem is populated before other containers start

### Deployment

On first run, you will want to use the Assets container to insert the test dataset into the 
device's filesystem. Do so with:
```
atym run assets ei-assets
```

Once that container executes, you can then deploy the inference and data containers to your device
using:
```bash
./run.sh
```

## Output

When running successfully, you'll see:

**Data Container Output:**
```
[DATA] Data publisher start
[DATA] Using sample directory: /testing
[DATA] Found 3 CBOR files
[DATA] Publish window 0 of sample "testing/idle.1.cbor.XXXX.cbor"
[DATA] Comparison for sample 'testing/idle.1.cbor...cbor' window 0:
[DATA]   expected='idle' predicted='idle' score=0.97563 -> MATCH
...
[DATA] Test results:
[DATA]   Total windows:   25
[DATA]   Correct windows: 24
[DATA]   Window accuracy: 96.00 %
```

**Classifier Container Output:**
```
[CLS] EI classifier subscriber starting up (closed-loop responder)...
[CLS] Listening for samples on topic 'ei/sample/raw'
[CLS] Publishing results on topic 'ei/result'
[CLS] [0.00241, 0.97563, ...]
[CLS] Published result: label=idle score=0.97563
```

## Architecture

### Container Communication

The three containers communicate via the Ocre messaging bus:

```
[Assets Container] (extracts CBOR files)
         |
         v
  /testing/ (filesystem)
         ^
         |
  [Data Container] -------[ei/sample/raw]-------> [Classifier Container]
  (publishes samples)                              (runs inference)
         ^
         |
     [ei/result]
  (receives predictions)
```

## Integration with Your Own Model

### Step 1: Export from Edge Impulse

1. Train your motion classification model on Edge Impulse
2. Go to **Deployment** > **C++ Library**
3. Select the model version you want to export
4. Download the exported `.zip` file
5. Extract it to this directory, replacing the `edge-impulse-sdk/` folder

The `model-parameters/` directory contains:
- `model_metadata.h` - Defines `EI_CLASSIFIER_RAW_SAMPLE_COUNT`, `EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE`, and label count
- `model_variables.h` - Model-specific constants

These are automatically generated when you export from Edge Impulse and should not need manual editing.

### Step 2: Prepare Test Dataset

1. In Edge Impulse, go to **Data acquisition** > **Testing**
2. Download the test samples as CBOR format files
3. Place all `.cbor` files in the `data/testing/` directory
4. The asset extractor will automatically embed these at build time

### Step 3: Build and Deploy

Follow the building and deployment instructions above. The CMake build system will:
1. Automatically discover CBOR files in `data/testing/`
2. Generate C headers to embed them via `gen_embedded_assets.py`
3. Link everything into the final containers

## Wasm/Zephyr Porting

This example is designed to run in a portable fashion and has been tested on Linux and Zephyr RTOS. 
If you wish to replicate these results with your own model, please consider the following.

### Model Export Settings

When exporting from Edge Impulse for Wasm targets:
- **Enable**: "Use xxd instead of INCBIN to link TFLite/ONNX files"
  - Wasm doesn't support INCBIN's binary embedding method
  - This setting is found in Dashboard > Experiments

### Compilation Flags

The following flags are critical for Wasm compatibility:

```
-fno-exceptions       # Wasm doesn't support C++ exceptions
-DEIDSP_SIGNAL_C_FN_POINTER  # Use C function pointers for signal processing
-DEI_C_LINKAGE        # Use C linkage for EI functions
-DUSE_CMSIS_NN=OFF    # CMSIS-NN optimizations not supported in WASI
```

These are already configured in the included `CMakeLists.txt`.

### Edge Impulse Runtime Considerations

- **Clock**: WASI only supports `CLOCK_MONOTONIC` for timing. If you use your own model, the included patch in `ei_classifier_porting.cpp` will need to be manually added.
- **Linking**: The Edge Impulse SDK is statically linked (not as a shared library) for portability.
- **Memory**: Wasm modules run in isolated memory; no cross-container memory sharing occurs.

## References

- [Edge Impulse C Library Documentation](https://docs.edgeimpulse.com/docs/run-inference/cpp-library/running-your-impulse-locally#using-the-library-from-c)
- [Edge Impulse Standalone Inferencing Example](https://github.com/edgeimpulse/example-standalone-inferencing)
- [Ocre Documentation](../../README.md)
