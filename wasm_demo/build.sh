#!/bin/bash
# Build script for WASM image processing demo

set -e  # Exit on error

echo "========================================="
echo "  WASM Image Processing Build Script"
echo "========================================="

# Check if Emscripten is available
if ! command -v emcc &> /dev/null; then
    echo "❌ Error: Emscripten compiler (emcc) not found!"
    echo "Please install and activate Emscripten:"
    echo "  source /path/to/emsdk/emsdk_env.sh"
    exit 1
fi

echo "✓ Emscripten compiler found: $(emcc --version | head -n 1)"

# Create output directory
OUTPUT_DIR="output"
mkdir -p "$OUTPUT_DIR"
echo "✓ Output directory: $OUTPUT_DIR"

# Find CImg.h (check local directory first, then parent)
CIMG_PATH=""
if [ -f "CImg.h" ]; then
    CIMG_PATH="."
elif [ -f "../cmake-build-debug/CImg/CImg.h" ]; then
    CIMG_PATH="../cmake-build-debug/CImg"
elif [ -f "../CImg.h" ]; then
    CIMG_PATH=".."
else
    echo "❌ Error: CImg.h not found!"
    echo "Please copy CImg.h to the wasm_demo directory or adjust CIMG_PATH in this script."
    exit 1
fi

echo "✓ CImg found: $CIMG_PATH/CImg.h"

# Compile with Emscripten
echo ""
echo "🔨 Compiling..."

emcc \
    wasm_bridge.cpp \
    simple_cimg_blur.cpp \
    -o "$OUTPUT_DIR/wasm_bridge.js" \
    -std=c++17 \
    -O3 \
    -pthread \
    -U_OPENMP \
    -Dcimg_display=0 \
    -I"$CIMG_PATH" \
    -msimd128 \
    -msse -msse2 \
    -sPTHREAD_POOL_SIZE=4 \
    -sALLOW_MEMORY_GROWTH=1 \
    -sINITIAL_MEMORY=67108864 \
    -sEXPORTED_FUNCTIONS='["_process_image","_wasm_get_thread_count","_malloc","_free"]' \
    -sEXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
    -sMODULARIZE=0 \
    -sEXPORT_NAME="Module" \
    -sENVIRONMENT=web,worker \
    --no-entry

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful!"
else
    echo "❌ Compilation failed!"
    exit 1
fi

# Copy HTML and JS files to output directory
echo ""
echo "📦 Copying web files..."
cp index.html "$OUTPUT_DIR/"
cp main.js "$OUTPUT_DIR/"
echo "✓ Files copied to $OUTPUT_DIR/"

# List output files
echo ""
echo "📁 Output files:"
ls -lh "$OUTPUT_DIR/"

echo ""
echo "========================================="
echo "  ✅ Build Complete!"
echo "========================================="
echo ""
echo "Next steps:"
echo "  1. Start server: python3 serve.py"
echo "  2. Open browser: http://localhost:8080"
echo ""


