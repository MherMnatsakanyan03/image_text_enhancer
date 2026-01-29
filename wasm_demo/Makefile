# Makefile for WASM Image Processing Demo with OpenMP and ITE
# Refactored to build everything (including libomp) with Emscripten

PROJECT_DIR := $(realpath .)
BUILD_DIR := $(PROJECT_DIR)/build-em

# Compiler Configuration
EMSDK_DIR := $(PROJECT_DIR)/emsdk
EMSDK_ENV := $(EMSDK_DIR)/emsdk_env.sh
SHELL := /bin/bash

# Helper to run commands in emsdk environment
RUN_EM := source $(EMSDK_ENV) > /dev/null 2>&1 &&

# OpenMP Configuration
OPENMP_DIR := $(PROJECT_DIR)/llvm-project/openmp
OPENMP_LIB := $(BUILD_DIR)/lib/libomp.a
OPENMP_INCLUDE_DIR := $(BUILD_DIR)/include

# ITE Configuration
ITE_PATH := ../src/lib
OUTPUT_DIR := output
CIMG_PATH := .

# Source Files
SOURCES := wasm_bridge.cpp \
           $(ITE_PATH)/ite.cpp \
           $(ITE_PATH)/core/integral_image.cpp \
           $(ITE_PATH)/color/grayscale.cpp \
           $(ITE_PATH)/color/contrast.cpp \
           $(ITE_PATH)/color/color.cpp \
           $(ITE_PATH)/binarization/binarization.cpp \
           $(ITE_PATH)/morphology/morphology.cpp \
           $(ITE_PATH)/geometry/geometry.cpp \
           $(ITE_PATH)/filters/filters.cpp \
           $(ITE_PATH)/io/image_io.cpp

# Compiler and Flags
CXX := emcc
CXXFLAGS := -std=c++17 -O3 -pthread -fopenmp=libomp -Dcimg_display=0 \
            -I$(CIMG_PATH) -I$(ITE_PATH) -I$(OPENMP_INCLUDE_DIR) \
            -L$(BUILD_DIR)/lib \
            -msimd128 \
			-U__SSE__ -U__SSE2__ # do not allow x86 flags

EMFLAGS := -s PTHREAD_POOL_SIZE=navigator.hardwareConcurrency \
           -s ALLOW_MEMORY_GROWTH=1 \
           -s INITIAL_MEMORY=536870912 \
           -s EXPORTED_FUNCTIONS='["_process_image", "_process_image_with_options", "_wasm_get_thread_count", "_malloc", "_free"]' \
           -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "HEAPU8"]' \
           -s MODULARIZE=0 \
           -s EXPORT_NAME="Module" \
           -s ENVIRONMENT=web,worker \
           --no-entry

OUTPUT := $(OUTPUT_DIR)/wasm_bridge.js
WEB_FILES := index.html main.js
OUTPUT_WEB := $(addprefix $(OUTPUT_DIR)/, $(WEB_FILES))

# Default target
all: $(OUTPUT) $(OUTPUT_WEB)

# Copy web files
$(OUTPUT_DIR)/%: %
	@mkdir -p $(OUTPUT_DIR)
	cp $< $@

# Compile the application (depends on OpenMP library and EMSDK)
$(OUTPUT): $(SOURCES) $(OPENMP_LIB) $(EMSDK_ENV) Makefile
	@mkdir -p $(OUTPUT_DIR)
	@echo "Building application..."
	$(RUN_EM) $(CXX) $(CXXFLAGS) $(EMFLAGS) $(SOURCES) $(OPENMP_LIB) -o $(OUTPUT)
	@echo "Build complete: $(OUTPUT)"

# Build the OpenMP runtime library using Emscripten
$(OPENMP_LIB): $(OPENMP_DIR) $(EMSDK_ENV)
	@echo "Building OpenMP Runtime..."
	@mkdir -p $(BUILD_DIR)
	$(RUN_EM) emcmake cmake \
        -DCMAKE_C_FLAGS="-DPAGESIZE=65536 -pthread -matomics -mbulk-memory" \
        -DCMAKE_CXX_FLAGS="-DPAGESIZE=65536 -pthread -matomics -mbulk-memory" \
        -DCMAKE_C_COMPILER=$(EMSDK_DIR)/upstream/emscripten/emcc \
        -DCMAKE_CXX_COMPILER=$(EMSDK_DIR)/upstream/emscripten/em++ \
        -DOPENMP_STANDALONE_BUILD=ON \
        -DOPENMP_ENABLE_OMPT_TOOLS=OFF \
        -DOPENMP_ENABLE_LIBOMPTARGET=OFF \
        -DLIBOMP_HAVE_OMPT_SUPPORT=OFF \
        -DLIBOMP_OMPT_SUPPORT=OFF \
        -DLIBOMP_OMPD_SUPPORT=OFF \
        -DLIBOMP_USE_DEBUGGER=OFF \
        -DLIBOMP_FORTRAN_MODULES=OFF \
        -DLIBOMP_ENABLE_SHARED=OFF \
        -DCMAKE_INSTALL_PREFIX=$(BUILD_DIR) \
        -DCMAKE_BUILD_TYPE=Release \
        -B $(BUILD_DIR) \
        -S $(OPENMP_DIR)
	$(RUN_EM) emmake make -C $(BUILD_DIR) -j
	$(RUN_EM) emmake make install -C $(BUILD_DIR)

# Emscripten SDK Setup
$(EMSDK_ENV):
	@echo "Setting up Emscripten SDK..."
	@if [ ! -d "$(EMSDK_DIR)" ]; then \
		echo "Cloning emsdk..."; \
		git clone https://github.com/emscripten-core/emsdk.git $(EMSDK_DIR); \
	fi
	cd $(EMSDK_DIR) && ./emsdk update
	cd $(EMSDK_DIR) && ./emsdk install latest
	cd $(EMSDK_DIR) && ./emsdk activate latest
	touch $(EMSDK_ENV)

# Check if OpenMP sources exist
$(OPENMP_DIR):
	$(error "OpenMP sources not found at $(OPENMP_DIR). Please clone llvm-project.")

clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_DIR)

.PHONY: all clean

