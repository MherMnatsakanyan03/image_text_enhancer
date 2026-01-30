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

        // Load default settings
        loadDefaultSettings();

        setStatus('Ready! Load an image to begin.', 'success');

    } catch (error) {
        setStatus(`Failed to load WASM: ${error.message}`, 'error');
        console.error('[WASM] Initialization error:', error);
    }
}

const OPT_SIZE = 92; // 23 fields * 4 bytes

function loadDefaultSettings() {
    const ptr = iteWasmModule._malloc(OPT_SIZE);
    iteWasmModule._get_default_options(ptr);
    
    // Helper to read form HEAP
    const getInt = (offset) => iteWasmModule.getValue(ptr + offset, 'i32');
    const getFloat = (offset) => iteWasmModule.getValue(ptr + offset, 'float');

    // Populate UI
    document.getElementById('boundary_conditions').value = getInt(0);
    document.getElementById('do_gaussian_blur').checked = getInt(4);
    document.getElementById('do_median_blur').checked = getInt(8);
    document.getElementById('do_adaptive_median').checked = getInt(12);
    document.getElementById('do_adaptive_gaussian_blur').checked = getInt(16);
    document.getElementById('do_color_pass').checked = getInt(20);
    document.getElementById('sigma').value = getFloat(24);
    document.getElementById('adaptive_sigma_low').value = getFloat(28);
    document.getElementById('adaptive_sigma_high').value = getFloat(32);
    document.getElementById('adaptive_edge_thresh').value = getFloat(36);
    document.getElementById('median_kernel_size').value = getInt(40);
    document.getElementById('median_threshold').value = getFloat(44);
    document.getElementById('adaptive_median_max_window').value = getInt(48);
    document.getElementById('diagonal_connections').checked = getInt(52);
    document.getElementById('do_erosion').checked = getInt(56);
    document.getElementById('do_dilation').checked = getInt(60);
    document.getElementById('do_despeckle').checked = getInt(64);
    document.getElementById('kernel_size').value = getInt(68);
    document.getElementById('despeckle_threshold').value = getInt(72);
    document.getElementById('do_deskew').checked = getInt(76);
    document.getElementById('sauvola_window_size').value = getInt(80);
    document.getElementById('sauvola_k').value = getFloat(84);
    document.getElementById('sauvola_delta').value = getFloat(88);

    iteWasmModule._free(ptr);
    console.log('[WASM] Default settings loaded');
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
    const ctx = canvas.getContext('2d', { willReadFrequently: true });

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
        const ctx = inputCanvas.getContext('2d', { willReadFrequently: true });

        const imageData = ctx.getImageData(0, 0, inputCanvas.width, inputCanvas.height);
        const width = imageData.width;
        const height = imageData.height;
        const pixels = imageData.data; // Uint8ClampedArray (RGBA)

        console.log(`[WASM] Processing ${width}×${height} image (${pixels.length} bytes)`);

        // Allocate memory for image
        const numBytes = pixels.length;
        const imgPtr = iteWasmModule._malloc(numBytes);
        
        // Allocate memory for options
        const optsPtr = iteWasmModule._malloc(OPT_SIZE);

        if (!imgPtr || !optsPtr) {
            throw new Error('Failed to allocate WASM memory');
        }

        // Fill options
        const setInt = (offset, val) => iteWasmModule.setValue(optsPtr + offset, val, 'i32');
        const setFloat = (offset, val) => iteWasmModule.setValue(optsPtr + offset, val, 'float');

        setInt(0, parseInt(document.getElementById('boundary_conditions').value));
        setInt(4, document.getElementById('do_gaussian_blur').checked ? 1 : 0);
        setInt(8, document.getElementById('do_median_blur').checked ? 1 : 0);
        setInt(12, document.getElementById('do_adaptive_median').checked ? 1 : 0);
        setInt(16, document.getElementById('do_adaptive_gaussian_blur').checked ? 1 : 0);
        setInt(20, document.getElementById('do_color_pass').checked ? 1 : 0);
        setFloat(24, parseFloat(document.getElementById('sigma').value));
        setFloat(28, parseFloat(document.getElementById('adaptive_sigma_low').value));
        setFloat(32, parseFloat(document.getElementById('adaptive_sigma_high').value));
        setFloat(36, parseFloat(document.getElementById('adaptive_edge_thresh').value));
        setInt(40, parseInt(document.getElementById('median_kernel_size').value));
        setFloat(44, parseFloat(document.getElementById('median_threshold').value));
        setInt(48, parseInt(document.getElementById('adaptive_median_max_window').value));
        setInt(52, document.getElementById('diagonal_connections').checked ? 1 : 0);
        setInt(56, document.getElementById('do_erosion').checked ? 1 : 0);
        setInt(60, document.getElementById('do_dilation').checked ? 1 : 0);
        setInt(64, document.getElementById('do_despeckle').checked ? 1 : 0);
        setInt(68, parseInt(document.getElementById('kernel_size').value));
        setInt(72, parseInt(document.getElementById('despeckle_threshold').value));
        setInt(76, document.getElementById('do_deskew').checked ? 1 : 0);
        setInt(80, parseInt(document.getElementById('sauvola_window_size').value));
        setFloat(84, parseFloat(document.getElementById('sauvola_k').value));
        setFloat(88, parseFloat(document.getElementById('sauvola_delta').value));

        console.log(`[WASM] Allocated ${numBytes} bytes at address ${imgPtr}`);

        // Copy pixel data to WASM memory
        iteWasmModule.HEAPU8.set(pixels, imgPtr);

        // Call WASM processing function
        console.log('[WASM] Calling process_image_with_options...');
        const time = iteWasmModule._process_image_with_options(imgPtr, width, height, optsPtr);

        // Read processed data back
        const processedPixels = new Uint8ClampedArray(iteWasmModule.HEAPU8.buffer, imgPtr, numBytes);

        // Copy to avoid issues when freeing memory
        const outputPixels = new Uint8ClampedArray(processedPixels);

        // Free WASM memory
        iteWasmModule._free(imgPtr);
        iteWasmModule._free(optsPtr);
        console.log('[WASM] Memory freed');

        // Display output image
        const outputCanvas = document.getElementById('outputCanvas');
        outputCanvas.width = width;
        outputCanvas.height = height;
        const outputCtx = outputCanvas.getContext('2d');
        const outputImageData = new ImageData(outputPixels, width, height);
        outputCtx.putImageData(outputImageData, 0, 0);

        const endTime = performance.now();
        const processTime = endTime - startTime;

        console.log(`[WASM] Processing complete in ${time} ms`);
        console.log(`[WASM] Measured in JS ${processTime} ms`);

        // Update stats
        document.getElementById('processTime').textContent = (time*1000).toFixed(2);;

        setStatus(`Processing including JS overhead completed in ${processTime} ms`, 'success');

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

