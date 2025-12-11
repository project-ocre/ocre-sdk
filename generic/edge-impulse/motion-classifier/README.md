# Edge Impulse Atym Container Example

- Leverages the EI C++ SDK and cribs from the provided ["standalone" C example](https://docs.edgeimpulse.com/docs/run-inference/cpp-library/running-your-impulse-locally#using-the-library-from-c). 
- Must clone the [Motion Classifier](https://docs.edgeimpulse.com/datasets/time-series/continuous-motion-recognition) project on Edge Impulse to replicate these results
- Builds 2 containers:
    - Classifier: 
        - Subscribes to data buffers on the Ocre message bus
        - Runs inference on a received buffer
        - Returns inference results on the Ocre message bus
    - Data:
        - Uses raw data samples from the test dataset from the EI model. These are placed in the filesystem in the root, in a folder called `testing` (should be copied directly)
        - Each file is read into an internal data structure and chunks of the expected sample window size are picked out (two options: random, or deterministic)
        - Each chunk is sent over the message bus to the classifier
        - Subscribes to the results from the classifier and collects overall inference performance data
        - Dumps performance data out once all dataset items are processed


## WASM Porting

This builds and runs an exported impulse locally on your machine. It compiles the inferencing library, and then links it to a C application. See the documentation at [Running your impulse locally -> Using the library from C]

There is also a [C++ version](https://github.com/edgeimpulse/example-standalone-inferencing) of this application.

### Compilation notes
The starting place for this was the Edge Impulse C SDK
- Don't compile with the `-g` flag. The build failed even before I started to move to WASM
- You must compile with `-fno-exceptions` for WASM compatibility.
- You must set the parameter "Use xxd instead of INCBIN to link TFLite/ONNX files" in Edge Impulse. WASM does not allow for the new method EI uses to embed the model binary in compiled code. Details below:
- `model_parameters/*` are where the configuration happens for which SDK functions are run during inference. Filters, parameters, model type, sensors, etc. 
- We need the flags `DEIDSP_SIGNAL_C_FN_POINTER` and `DEI_C_LINKAGE` for the build process. These are enabled in CMakeLists already as our main app is in C.

```
// NOTE: As of February 2025 we've changed the default way to include binary model files.
// Instead of embedding the file as as a byte array in this file, we now use INCBIN instead;
// as it yields much shorter link times, and lower memory usage when building applications.
// If this causes issues to your builds (maybe the reason why you're visiting this file)
// you can go to **Dashboard > Experiments > Use xxd instead of INCBIN to link TFLite/ONNX files**
// to re-enable the previous behavior. Please also let us know at the forum (https://forum.edgeimpulse.com/)
// or through your Solutions Engineers (if applicable) what target you're building for, and what
// issues arise.
```

- WASM only supports CLOCK_MONOTONIC. We added a check for `__wasi__` to `porting/posix/ei_classifier_porting.cpp` as there was already a selector for using `CLOCK_MONOTINIC` with a different condition.
- We explicitly disable CMSIS-NN optimizations since WASI does not support them. May not be needed for the POSIX port, but wanted to make sure they are off.
- The original build system compiles a `.so` for the edge impulse sdk. This is now statically linked.

Other notes:
- Instead of loading data from a file, I hardcoded a sample into `main.c` to test with. One less `stdlib` dependency.
