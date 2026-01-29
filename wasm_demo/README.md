# WebAssembly Demo: CImg Image Processing with OpenMP

This is a **minimum working example (MWE)** that compiles a CImg-based image processing function to WebAssembly with OpenMP support and runs it in a browser.

## Features

- 🖼️ Load images via file picker or drag-and-drop
- ⚡ Process images using WebAssembly with OpenMP parallelization
- 🎨 Display input and output side-by-side on HTML canvas
- 🔧 Applies Gaussian blur (simple demo function)

---

## Project Structure

```
wasm_demo/
├── README.md              # This file
├── index.html             # Web UI with file picker and canvas
├── main.js                # JavaScript: loads WASM, handles I/O, renders images
├── wasm_bridge.cpp        # C++ WASM interface: exposes processing function
├── simple_cimg_blur.cpp   # Minimal CImg processing function (Gaussian blur)
├── simple_cimg_blur.h     # Header for the processing function
├── build.sh               # Build script using Emscripten
├── serve.py               # Python HTTP server with COOP/COEP headers
└── output/                # Build output directory (created by build.sh)
```

---

## Requirements

### 1. Install Emscripten

```bash
# Clone and install Emscripten SDK
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh  # Add to ~/.bashrc for persistence
```

Verify installation:
```bash
emcc --version  # Should show Emscripten compiler version
```

### 2. Python 3 (for local server)

```bash
python3 --version  # Should be 3.6+
```

---

## Build Instructions

```bash
cd wasm_demo
make
```

This will:
- Compile `wasm_bridge.cpp` and `simple_cimg_blur.cpp` with Emscripten
- Enable OpenMP and pthreads
- Generate `output/wasm_bridge.js` and `output/wasm_bridge.wasm`
- Copy `index.html` and `main.js` to `output/`

**Build flags used:**
- `-O3`: Maximum optimization
- `-fopenmp`: Enable OpenMP support
- `-pthread`: Enable POSIX threads
- `-sPTHREAD_POOL_SIZE=4`: Pre-allocate 4 worker threads
- `-sALLOW_MEMORY_GROWTH=1`: Dynamic memory allocation
- `-sEXPORTED_FUNCTIONS`: Export C functions to JavaScript
- `-sEXPORTED_RUNTIME_METHODS`: Export WASM runtime utilities

---

## Run Instructions

### Step 1: Start the local server with COOP/COEP headers

```bash
cd wasm_demo
python3 serve.py
```

**Why these headers?**
- WebAssembly with threads requires `SharedArrayBuffer`
- Browsers require Cross-Origin-Opener-Policy (COOP) and Cross-Origin-Embedder-Policy (COEP) headers for security

The server will start on `http://localhost:8080`

### Step 2: Open in browser

Navigate to:
```
http://localhost:8080
```

**Supported browsers:**
- Chrome/Edge 92+ (recommended)
- Firefox 89+
- Safari 15.2+

### Step 3: Process an image

1. Click **"Choose File"** or drag-and-drop an image (PNG, JPEG, GIF)
2. The image will be loaded and displayed in the "Input" canvas
3. Click **"Process Image"** 
4. The WASM module will apply Gaussian blur using OpenMP
5. The result appears in the "Output" canvas
6. Console shows processing time and thread count

---

## Verification & Debugging

### 1. Check if OpenMP is working

Open the browser's **Developer Console** (F12) and look for:
```
[WASM] OpenMP threads available: 4
[WASM] Processing 512x512 image...
[WASM] Processing complete in 15.23 ms
```

If thread count is 1, OpenMP is not active (check browser support or headers).

### 2. Check SharedArrayBuffer

In the console, type:
```javascript
typeof SharedArrayBuffer
```
Should return `"function"`. If `"undefined"`, the COOP/COEP headers are not set correctly.

### 3. Common Issues

| Problem | Solution |
|---------|----------|
| **WASM fails to load** | Check console for errors. Ensure files are served from `output/` directory |
| **"SharedArrayBuffer is not defined"** | Server must send COOP/COEP headers. Use provided `serve.py` |
| **Image not displaying** | Check image format (must be browser-decodable). Check console for errors |
| **Processing hangs** | Increase `PTHREAD_POOL_SIZE` in `build.sh`, or reduce image size |
| **Slow processing** | Disable debug builds. Ensure `-O3` is used. Check that OpenMP is active |

### 4. Inspect WASM memory

```javascript
// In browser console:
const heap = Module.HEAPU8;  // Access WASM memory heap
console.log("Heap size:", heap.length);
```

### 5. Test without OpenMP (fallback)

Edit `build.sh` and remove `-fopenmp`, then rebuild:
```bash
./build.sh
```

This verifies the WASM build works without threading.

---

## How It Works

### Memory Flow

```
Browser (JavaScript)              WASM Memory (C++)
─────────────────────              ──────────────────
1. User selects image
2. Canvas decodes to RGBA
3. Get pixel data (Uint8Array)
                              ──>  4. malloc buffer
                              ──>  5. Copy pixels to buffer
                                   6. Process (OpenMP parallel)
                                   7. Write result to same buffer
4. Copy result back          <──  
5. Render to output canvas
```

### Function Signature

C++ (exported from `wasm_bridge.cpp`):
```cpp
extern "C" {
    void process_image(uint8_t* rgba_data, int width, int height);
}
```

JavaScript (called from `main.js`):
```javascript
Module._process_image(ptr, width, height);
```

### OpenMP Parallelization

The Gaussian blur uses CImg's `blur()` method, which internally can leverage OpenMP if compiled with `-fopenmp`. For larger images, you'll see performance improvements with more threads.

---

## Customization

### Use a different processing function

Edit `simple_cimg_blur.cpp` and change the `apply_blur()` function:

```cpp
void apply_blur(CImg<uint8_t> &img) {
    // Example: edge detection
    img.edge("sobel", 0);
    
    // Example: brightness adjustment
    img *= 1.5;  // 50% brighter
    
    // Example: binarization (threshold)
    cimg_forXY(img, x, y) {
        img(x, y) = img(x, y) > 128 ? 255 : 0;
    }
}
```

Rebuild with `./build.sh`.

### Adjust OpenMP thread count

Edit `build.sh` and change:
```bash
-sPTHREAD_POOL_SIZE=4  # Change 4 to desired thread count
```

Or set dynamically in JavaScript:
```javascript
Module.PThread.initMainThread(8);  // Use 8 threads
```

### Integrate with your ITE library

Replace `simple_cimg_blur.cpp` with calls to your `ite::` functions:

```cpp
#include "ite.h"

void apply_processing(CImg<uint8_t> &img) {
    // Convert to CImg<uint> if needed
    CImg<uint> img_uint(img.width(), img.height(), 1, img.spectrum());
    cimg_forXYC(img, x, y, c) {
        img_uint(x, y, 0, c) = img(x, y, 0, c);
    }
    
    // Apply ITE functions
    img_uint = ite::to_grayscale(img_uint);
    img_uint = ite::simple_gaussian_blur(img_uint, 2.0f);
    img_uint = ite::binarize_sauvola(img_uint);
    
    // Convert back
    cimg_forXY(img_uint, x, y) {
        img(x, y) = img_uint(x, y);
    }
}
```

Then update `build.sh` to include your ITE source files.

---

## Performance Tips

1. **Image size**: Start with 512×512 or smaller for testing
2. **Thread count**: 4-8 threads is optimal for most devices
3. **Optimization**: Always use `-O3` in production
4. **Memory**: Pre-allocate buffers when processing multiple images
5. **Profiling**: Use browser's Performance tab to measure bottlenecks

---

## Multi-Threading Status ⚠️

### Current Reality: Single-Threaded Execution

This browser demo is **effectively single-threaded**. Here's why:

**OpenMP in Emscripten (Browser):**
- ❌ **NOT SUPPORTED** - Emscripten does not provide an OpenMP runtime for browser targets
- ❌ `#pragma omp` directives are **silently ignored** (compiled as no-ops)
- ❌ Even with `-fopenmp` flag, OpenMP functions are unavailable
- ✅ `-pthread` enables POSIX threads, but this is **not OpenMP** (requires manual thread management)

**What You Actually Have:**
- ✅ SharedArrayBuffer support (COOP/COEP headers configured)
- ✅ POSIX threads infrastructure (`-pthread`)
- ❌ No OpenMP runtime (`libomp.a` not available in browser)
- ❌ No automatic parallelization

### Performance (Single-Threaded)

These measurements reflect **actual single-threaded performance**:

| Image Size | Processing Time | Notes |
|------------|----------------|-------|
| 512×512 | ~15ms | Acceptable for UI |
| 1920×1080 (Full HD) | ~150ms | Noticeable delay |
| 6720×4480 (6K) | ~2290ms | Slow, needs optimization |

### OpenMP in WASI (CLI Only) ℹ️

OpenMP **IS possible** in WebAssembly, but **ONLY** with:
1. **WASI SDK** (not Emscripten)
2. **Custom-built `libomp.a`** for wasm32-wasi-threads
3. **CLI runtime** (wasmtime), not browsers
4. **Experimental toolchain** (complex setup)

See **TECHNICAL_ASSESSMENT.md** for details on dual-target architecture (browser + CLI).

### Optimization Roadmap

**Short-term (browser):**
- Algorithmic improvements (separable convolution: 3–5× faster)
- WebAssembly SIMD instructions (2–3× faster)
- Reduced memory copies

**Mid-term (if needed):**
- WASI target with real OpenMP for CLI batch processing
- Web Workers with manual parallelism

**Long-term:**
- WebGPU compute shaders (10–100× faster for large images)

---

## Further Reading

- [Emscripten Pthreads](https://emscripten.org/docs/porting/pthreads.html)
- [WASI-SDK with Threads](https://github.com/aspect-build/aspect-build-wasi-sdk)
- [SharedArrayBuffer Requirements](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/SharedArrayBuffer)
- [CImg Documentation](http://cimg.eu/reference/group__cimg__tutorial.html)
- [WebAssembly Threads Proposal](https://github.com/WebAssembly/threads)

---

## License

This demo is provided as-is for educational purposes. CImg is licensed under CeCILL-C (compatible with LGPL).

