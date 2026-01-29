# Makefile for WASM Image Processing Demo with OpenMP and ITE
# Refactored to support multi-threading and ITE integration

# Configuration
OUTPUT_DIR = output
CIMG_PATH = .
ITE_PATH = ../src/lib
# OpenMP paths based on build_openmp_emscripten.sh output
OPENMP_BUILD_DIR = build-openmp/lib
OPENMP_INCLUDE_DIR = build-openmp/include

# Output files
WASM_OUTPUT = $(OUTPUT_DIR)/wasm_bridge.js
WEB_FILES = index.html main.js

# Source files
# Bridge
SOURCES = wasm_bridge.cpp

# ITE Library Sources
SOURCES += $(ITE_PATH)/ite.cpp \
           $(ITE_PATH)/core/integral_image.cpp \
           $(ITE_PATH)/color/grayscale.cpp \
           $(ITE_PATH)/color/contrast.cpp \
           $(ITE_PATH)/color/color.cpp \
           $(ITE_PATH)/binarization/binarization.cpp \
           $(ITE_PATH)/morphology/morphology.cpp \
           $(ITE_PATH)/geometry/geometry.cpp \
           $(ITE_PATH)/filters/filters.cpp \
           $(ITE_PATH)/io/image_io.cpp

# Compiler and flags
CXX = emcc
# Flags: C++17, Opts, Pthreads, OpenMP, Includes, SIMD
CXXFLAGS = -std=c++17 -O2 -pthread -fopenmp=libomp -Dcimg_display=0 \
           -I$(CIMG_PATH) -I$(ITE_PATH) -I$(OPENMP_INCLUDE_DIR) \
           -L$(OPENMP_BUILD_DIR) -msimd128 -msse -msse2

# Emscripten-specific flags
EMFLAGS = \
	-s PTHREAD_POOL_SIZE=navigator.hardwareConcurrency \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s INITIAL_MEMORY=67108864 \
	-s EXPORTED_FUNCTIONS='["_process_image", "_process_image_with_options", "_wasm_get_thread_count", "_malloc", "_free"]' \
	-s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
	-s MODULARIZE=0 \
	-s EXPORT_NAME="Module" \
	-s ENVIRONMENT=web,worker \
	--no-entry

# Default target
.PHONY: all
all: libomp check-emcc check-cimg build copy-files success

# Build OpenMP runtime using the provided script
.PHONY: libomp
libomp:
	@echo "Building OpenMP runtime..."
	@if [ ! -f "$(OPENMP_BUILD_DIR)/libomp.a" ]; then \
		bash build_openmp_emscripten.sh; \
	else \
		echo "✓ LibOMP already built."; \
	fi

# Check if Emscripten is available
.PHONY: check-emcc
check-emcc:
	@which emcc > /dev/null || (echo "❌ Error: Emscripten (emcc) not found!" && exit 1)

# Check if CImg exists
.PHONY: check-cimg
check-cimg:
	@test -f $(CIMG_PATH)/CImg.h || (echo "❌ Error: CImg.h not found!" && exit 1)

# Create output directory
$(OUTPUT_DIR):
	@mkdir -p $(OUTPUT_DIR)

# Build the WASM module
.PHONY: build
build: $(OUTPUT_DIR)
	@echo "🔨 Building WASM module..."
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(WASM_OUTPUT) $(EMFLAGS)
	@echo "✓ Build complete: $(WASM_OUTPUT)"

# Copy web files
.PHONY: copy-files
copy-files:
	@echo "📦 Copying web files to $(OUTPUT_DIR)/"
	@cp $(WEB_FILES) $(OUTPUT_DIR)/
	@echo "✓ Files copied"

# Success message
.PHONY: success
success:
	@echo ""
	@echo "========================================="
	@echo "  ✅ Build Complete!"
	@echo "========================================="
	@echo "Run: make serve"

# Start development server
.PHONY: serve
serve:
	@echo "🚀 Starting development server..."
	@python3 serve.py

# Clean
.PHONY: clean
clean:
	rm -rf $(OUTPUT_DIR)

