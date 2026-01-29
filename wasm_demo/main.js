// main.js - WebAssembly Image Processing Interface

let iteWasmModule = null;
let currentImageData = null;

// Status display helper
function setStatus(message, type = 'info') {
    const statusEl = document.getElementById('status');
    statusEl.textContent = message;
    statusEl.className = type;
    console.log(`[${type.toUpperCase()}] ${message}`);
}

// Initialize WASM module
async function initWasm() {
    setStatus('Loading WebAssembly module...', 'info');

    try {
        // Check for SharedArrayBuffer support (required for threads)
        if (typeof SharedArrayBuffer === 'undefined') {
            throw new Error('SharedArrayBuffer is not available. Server must send COOP and COEP headers.');
        }

        console.log('[WASM] SharedArrayBuffer available ✓');

        // Load the WASM module
        iteWasmModule = await createModule();

        console.log('[WASM] Module loaded successfully');

        // Check OpenMP thread availability
        const threadCount = iteWasmModule._wasm_get_thread_count();
        console.log(`[WASM] OpenMP threads available: ${threadCount}`);
        document.getElementById('threadCount').textContent = threadCount;

        setStatus('Ready! Load an image to begin.', 'success');

    } catch (error) {
        setStatus(`Failed to load WASM: ${error.message}`, 'error');
        console.error('[WASM] Initialization error:', error);
    }
}

// Load image from file
function loadImage(file) {
    setStatus('Loading image...', 'info');

    const reader = new FileReader();
    reader.onload = (e) => {
        const img = new Image();
        img.onload = () => {
            displayInputImage(img);
            currentImageData = img;
            document.getElementById('processBtn').disabled = false;
            setStatus('Image loaded. Click "Process Image" to apply blur.', 'success');

            // Update stats
            document.getElementById('imageSize').textContent = `${img.width}×${img.height}`;
            document.getElementById('stats').style.display = 'flex';
        };
        img.onerror = () => {
            setStatus('Failed to load image. Invalid format?', 'error');
        };
        img.src = e.target.result;
    };
    reader.onerror = () => {
        setStatus('Failed to read file.', 'error');
    };
    reader.readAsDataURL(file);
}

// Display input image on canvas
function displayInputImage(img) {
    const canvas = document.getElementById('inputCanvas');
    const ctx = canvas.getContext('2d');

    canvas.width = img.width;
    canvas.height = img.height;

    ctx.drawImage(img, 0, 0);
    console.log(`[Canvas] Input image displayed: ${img.width}×${img.height}`);
}

// Process image with WASM
function processImage() {
    if (!iteWasmModule || !currentImageData) {
        setStatus('WASM module not ready or no image loaded.', 'error');
        return;
    }

    setStatus('Processing...', 'info');

    const startTime = performance.now();

    try {
        // Get input image data from canvas
        const inputCanvas = document.getElementById('inputCanvas');
        const ctx = inputCanvas.getContext('2d');
        const imageData = ctx.getImageData(0, 0, inputCanvas.width, inputCanvas.height);

        const width = imageData.width;
        const height = imageData.height;
        const pixels = imageData.data; // Uint8ClampedArray (RGBA)

        console.log(`[WASM] Processing ${width}×${height} image (${pixels.length} bytes)`);

        // Allocate memory in WASM heap
        const numBytes = pixels.length;
        const ptr = iteWasmModule._malloc(numBytes);

        if (!ptr) {
            throw new Error('Failed to allocate WASM memory');
        }

        console.log(`[WASM] Allocated ${numBytes} bytes at address ${ptr}`);

        // Copy pixel data to WASM memory
        iteWasmModule.HEAPU8.set(pixels, ptr);

        // Call WASM processing function
        console.log('[WASM] Calling process_image...');
        iteWasmModule._process_image(ptr, width, height);

        // Read processed data back
        const processedPixels = new Uint8ClampedArray(iteWasmModule.HEAPU8.buffer, ptr, numBytes);

        // Copy to avoid issues when freeing memory
        const outputPixels = new Uint8ClampedArray(processedPixels);

        // Free WASM memory
        iteWasmModule._free(ptr);
        console.log('[WASM] Memory freed');

        // Display output image
        const outputCanvas = document.getElementById('outputCanvas');
        outputCanvas.width = width;
        outputCanvas.height = height;
        const outputCtx = outputCanvas.getContext('2d');
        const outputImageData = new ImageData(outputPixels, width, height);
        outputCtx.putImageData(outputImageData, 0, 0);

        const endTime = performance.now();
        const processTime = (endTime - startTime).toFixed(2);

        console.log(`[WASM] Processing complete in ${processTime} ms`);

        // Update stats
        document.getElementById('processTime').textContent = processTime;

        setStatus(`Processing complete in ${processTime} ms`, 'success');

    } catch (error) {
        setStatus(`Processing failed: ${error.message}`, 'error');
        console.error('[WASM] Processing error:', error);
    }
}

// File input handler
document.getElementById('fileInput').addEventListener('change', (e) => {
    const file = e.target.files[0];
    if (file) {
        loadImage(file);
    }
});

// Process button handler
document.getElementById('processBtn').addEventListener('click', processImage);

// Drag and drop handlers
const dropZone = document.getElementById('dropZone');

dropZone.addEventListener('dragover', (e) => {
    e.preventDefault();
    dropZone.classList.add('drag-over');
});

dropZone.addEventListener('dragleave', () => {
    dropZone.classList.remove('drag-over');
});

dropZone.addEventListener('drop', (e) => {
    e.preventDefault();
    dropZone.classList.remove('drag-over');

    const file = e.dataTransfer.files[0];
    if (file && file.type.startsWith('image/')) {
        loadImage(file);
    } else {
        setStatus('Please drop an image file.', 'error');
    }
});

// WASM module factory (will be replaced by Emscripten's generated code)
async function createModule() {
    // This will load wasm_bridge.js which defines the Module
    return new Promise((resolve, reject) => {
        // Set up the Module configuration BEFORE loading the script
        window.Module = {
            onRuntimeInitialized: function() {
                console.log('[WASM] Runtime initialized');
                resolve(window.Module);
            }
        };

        const script = document.createElement('script');
        script.src = 'wasm_bridge.js';
        script.onload = () => {
            console.log('[WASM] Script loaded, waiting for runtime initialization...');
            // If the module is already initialized (rare race condition), resolve immediately
            if (window.Module && window.Module.calledRun) {
                console.log('[WASM] Module already initialized');
                resolve(window.Module);
            }
        };
        script.onerror = () => {
            reject(new Error('Failed to load wasm_bridge.js'));
        };
        document.body.appendChild(script);
    });
}

// Initialize on page load
window.addEventListener('load', initWasm);

