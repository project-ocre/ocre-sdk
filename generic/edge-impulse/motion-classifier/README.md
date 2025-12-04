# Edge Impulse Example: stand-alone inferencing (C)

This builds and runs an exported impulse locally on your machine. It compiles the inferencing library, and then links it to a C application. See the documentation at [Running your impulse locally -> Using the library from C](https://docs.edgeimpulse.com/docs/run-inference/cpp-library/running-your-impulse-locally#using-the-library-from-c). 

There is also a [C++ version](https://github.com/edgeimpulse/example-standalone-inferencing) of this application.

### EI Notes
- `model_parameters/*` are where the configuration happens for which SDK functions are run during inference. Filters, parameters, model type, sensors, etc. 
- We need the flags `DEIDSP_SIGNAL_C_FN_POINTER` and `DEI_C_LINKAGE` for the build process. These are enabled in CMakeLists already as our main app is in C.


### WASM Porting notes
The starting place for this was the Edge Impulse C SDK
- Don't compile with the `-g` flag. The build failed even before I started to move to WASM
- You must compile with `-fno-exceptions` for WASM compatibility.
- You must set the parameter "Use xxd instead of INCBIN to link TFLite/ONNX files" in Edge Impulse. WASM does not allow for the new method EI uses to embed the model binary in compiled code. Details below:

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

- WASM only supports CLOCK_MONOTONIC. This problem will go away with a proper Atym/WASM port. (`porting/posix/ei_classifier_porting.cpp`)
- I set up my model for an ARM core, and it tries to use CMSIS-NN optimizations. I have turned those off in the makefile. (to be tested)
- The original build system compiles a `.so` for the edge impulse sdk. This is now statically linked.

Other notes:
- Instead of loading data from a file, I hardcoded a sample into `main.c` to test with. One less `stdlib` dependency.
