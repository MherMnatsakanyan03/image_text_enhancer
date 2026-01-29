#!/bin/bash
# Build OpenMP runtime library with Emscripten
# Based on: https://github.com/abrown/wasm-openmp-examples/blob/main/emscripten.sh

set -e

PROJECT_DIR=$(pwd)
BUILD_DIR=build-openmp
OPENMP_DIR=${OPENMP_DIR:=../cmake-build-debug/_deps/catch2-subbuild}  # Adjust if needed

# Check if LLVM OpenMP sources exist
if [ ! -d "llvm-project/openmp" ]; then
    echo "❌ Error: OpenMP sources not found!"
    echo ""
    echo "Please clone LLVM OpenMP sources:"
    echo "  git clone --depth=1 --branch=release/18.x https://github.com/llvm/llvm-project.git"
    echo ""
    echo "Or if you want the full repo:"
    echo "  git submodule add https://github.com/llvm/llvm-project.git"
    echo "  git submodule update --init --depth=1"
    exit 1
fi

OPENMP_DIR=$PROJECT_DIR/llvm-project/openmp
OPENMP_LIB=$BUILD_DIR/lib/libomp.a

echo "========================================="
echo "  Building OpenMP Runtime for Emscripten"
echo "========================================="
echo ""
echo "OpenMP source: $OPENMP_DIR"
echo "Build dir: $BUILD_DIR"
echo "Output: $OPENMP_LIB"
echo ""

# Check if Emscripten is available
if ! command -v emcc &> /dev/null; then
    echo "❌ Error: Emscripten not found!"
    echo "Activate it with: source /path/to/emsdk/emsdk_env.sh"
    exit 1
fi

echo "✓ Using Emscripten: $(emcc --version | head -n 1)"
echo ""

# Create build directory
mkdir -p $BUILD_DIR

# Configure OpenMP with Emscripten's CMake wrapper
echo "🔧 Configuring OpenMP build..."
export CFLAGS="-pthread"
export CXXFLAGS="-pthread"

emcmake cmake \
    -DOPENMP_STANDALONE_BUILD=ON \
    -DOPENMP_ENABLE_OMPT_TOOLS=OFF \
    -DOPENMP_ENABLE_LIBOMPTARGET=OFF \
    -DLIBOMP_HAVE_OMPT_SUPPORT=OFF \
    -DLIBOMP_OMPT_SUPPORT=OFF \
    -DLIBOMP_OMPD_SUPPORT=OFF \
    -DLIBOMP_USE_DEBUGGER=OFF \
    -DLIBOMP_FORTRAN_MODULES=OFF \
    -DLIBOMP_ENABLE_SHARED=OFF \
    -DCMAKE_INSTALL_PREFIX=$BUILD_DIR \
    -B $BUILD_DIR \
    -S $OPENMP_DIR

echo ""
echo "🔨 Building OpenMP runtime..."
emmake make -C $BUILD_DIR -j

echo ""
echo "📦 Installing OpenMP runtime..."
emmake make install -C $BUILD_DIR

echo ""
echo "========================================="
echo "  ✅ OpenMP Runtime Build Complete!"
echo "========================================="
echo ""
echo "Output: $OPENMP_LIB"
ls -lh $OPENMP_LIB
echo ""
echo "Headers installed in: $BUILD_DIR/include/"
ls -la $BUILD_DIR/include/
echo ""
echo "Next steps:"
echo "  1. Update your Makefile to use this OpenMP library"
echo "  2. Add flags: -I$BUILD_DIR/include -L$BUILD_DIR/lib -lomp -fopenmp=libomp"
echo "  3. Rebuild your WASM module: make rebuild"
echo ""

