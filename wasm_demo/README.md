# WebAssembly Demo: Image Text Enhancer (ITE)

This demo compiles the **Image Text Enhancer (ITE)** C++ library to WebAssembly, running entirely in the browser. It supports multi-threaded processing using **OpenMP** (via web workers) and SIMD optimizations.

## Features

- 🖼️ **Full Image Processing Pipeline**: Access to ITE's core features including:
  - Adaptive Gaussian & Median Blurring
  - Sauvola Binarization
  - Color Enhancement
  - Morphology (Erosion/Dilation/Despeckling)
  - Deskewing
- ⚡ **High Performance**: 
  - **OpenMP** parallelization (multithreaded WASM)
  - **SIMD128** optimizations enabled
- 🔧 **Interactive UI**: Adjust processing parameters in real-time.

---

## Project Structure

```
wasm_demo/
├── README.md               # This file
├── index.html              # Web UI
├── main.js                 # JavaScript glue code
├── wasm_bridge.cpp         # C++ interface binding ITE to JS
├── Makefile                # Build configuration
├── serve.py                # Python HTTP server (COOP/COEP headers)
├── emsdk/                  # Emscripten SDK (local dependency)
├── llvm-project/           # LLVM project (for OpenMP lib if needed)
└── output/                 # Build artifacts (created by make)
```

---

## Requirements

In addition to the regular build tools for your platform (i.e. C/C++ toolchain), you will need **Python 3** for the local development server.

The git repository includes `emsdk` and the `llvm-project` at compatible commits. It is important to match them, so that the ABI works.
If necessary, you can change the `emsdk` version at the top of the `Makefile` and you can change the `llvm-project` version by simply checking out the appropriate commit.

---

## Build Instructions

Use the provided `Makefile` to compile the project.

### 1. Standard Build (OpenMP Enabled)

This is the default target. It requires a browser with `SharedArrayBuffer` support.

```bash
make
# OR
make omp
```

### 2. Single-Threaded Build (No OpenMP)

Useful for compatibility with environments that don't support `SharedArrayBuffer` or cross-origin isolation.

```bash
make noomp
```

### Build Artifacts

Output files are generated in the `output/` directory:
- `wasm_bridge.js`: Emscripten glue code
- `wasm_bridge.wasm`: Compiled WebAssembly binary
- `index.html` & `main.js`: Copied for deployment

### Key Build Flags used
- `-O3`: Maximum optimization
- `-msimd128`: Enable SIMD instructions
- `-fopenmp`: Enable OpenMP (for threaded build)
- `PTHREAD_POOL_SIZE=navigator.hardwareConcurrency`: Automatically utilize available CPU cores
- `INITIAL_MEMORY=4GB`: Pre-allocated memory to handle large images (ALLOW_MEMORY_GROWTH disabled for performance/stability with large initial heap)

---

## Run Instructions

### Start the Local Server

WebAssembly threads require **Cross-Origin-Opener-Policy (COOP)** and **Cross-Origin-Embedder-Policy (COEP)** headers. The included `serve.py` handles this.

```bash
cd wasm_demo
python3 serve.py
```

### Open in Browser

Navigate to:
```
http://localhost:8080
```

Load an image and adjust the controls to test the enhancement pipeline.

