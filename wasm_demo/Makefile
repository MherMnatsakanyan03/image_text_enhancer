# Makefile for WASM Image Processing Demo with optional OpenMP (ITE + CImg)
# Build with OpenMP (default):  make
# Build without OpenMP:         make OPENMP=0
# Convenience targets:          make omp | make noomp

PROJECT_DIR := $(realpath .)
BUILD_DIR   := $(PROJECT_DIR)/build-em
OUTPUT_DIR  := output

# Compiler Configuration
EMSDK_DIR := $(PROJECT_DIR)/emsdk
EMSDK_ENV := $(EMSDK_DIR)/emsdk_env.sh
SHELL := /bin/bash

# Helper to run commands in emsdk environment
RUN_EM := source $(EMSDK_ENV) > /dev/null 2>&1 &&

# Optional OpenMP switch (1=on, 0=off)
OPENMP ?= 1

# OpenMP Runtime build (only used when OPENMP=1)
OPENMP_DIR         := $(PROJECT_DIR)/llvm-project/openmp
OPENMP_LIB         := $(BUILD_DIR)/lib/libomp.a
OPENMP_INCLUDE_DIR := $(BUILD_DIR)/include

# ITE Configuration
ITE_PATH  := ../src/lib
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

# Toolchain
CXX := em++

# Common compiler flags (both builds)
CXXFLAGS_COMMON := -std=c++17 -O3 -Dcimg_display=0 \
                   -I$(CIMG_PATH) -I$(ITE_PATH) \
                   -msimd128 \
                   -U__SSE__ -U__SSE2__ \
                   -Wno-unknown-pragmas

# Common emscripten flags (both builds)
EMFLAGS_COMMON := -s ALLOW_MEMORY_GROWTH=0 \
                  -s ALLOW_TABLE_GROWTH=1 \
                  -s INITIAL_MEMORY=2147483648 \
                  -s EXPORTED_FUNCTIONS='["_process_image", "_process_image_with_options", "_get_default_options", "_wasm_get_thread_count", "_malloc", "_free"]' \
                  -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "HEAPU8", "getValue", "setValue"]' \
                  -s MODULARIZE=0 \
                  -s EXPORT_NAME="Module" \
                  -s ENVIRONMENT=web,worker \
                  --no-entry

#536870912

# OpenMP/threaded build flags
CXXFLAGS_OMP := -pthread -fopenmp=libomp -I$(OPENMP_INCLUDE_DIR)
EMFLAGS_OMP  := -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=navigator.hardwareConcurrency

# No-OpenMP build flags (single-thread)
CXXFLAGS_NOOMP :=
EMFLAGS_NOOMP  := -s USE_PTHREADS=0

# Pick flags/libs based on OPENMP switch
ifeq ($(OPENMP),1)
  CXXFLAGS := $(CXXFLAGS_COMMON) $(CXXFLAGS_OMP)
  EMFLAGS  := $(EMFLAGS_COMMON)  $(EMFLAGS_OMP)
  OMP_DEPS := $(OPENMP_LIB) $(OPENMP_DIR)
  OMP_LIBS := $(OPENMP_LIB)
else
  CXXFLAGS := $(CXXFLAGS_COMMON) $(CXXFLAGS_NOOMP)
  EMFLAGS  := $(EMFLAGS_COMMON)  $(EMFLAGS_NOOMP)
  OMP_DEPS :=
  OMP_LIBS :=
endif

OUTPUT      := $(OUTPUT_DIR)/wasm_bridge.js
WEB_FILES   := index.html main.js
OUTPUT_WEB  := $(addprefix $(OUTPUT_DIR)/, $(WEB_FILES))

# Default target
all: $(OUTPUT) $(OUTPUT_WEB)

# Convenience targets
omp:
	@$(MAKE) OPENMP=1 all

noomp:
	@$(MAKE) OPENMP=0 all

# Copy web files
$(OUTPUT_DIR)/%: %
	@mkdir -p $(OUTPUT_DIR)
	cp $< $@

# Compile the application
$(OUTPUT): $(SOURCES) $(EMSDK_ENV) Makefile $(OMP_DEPS)
	@mkdir -p $(OUTPUT_DIR)
	@echo "Building application (OPENMP=$(OPENMP))..."
	$(RUN_EM) $(CXX) $(CXXFLAGS) $(EMFLAGS) $(SOURCES) $(OMP_LIBS) -o $(OUTPUT)
	@echo "Build complete: $(OUTPUT)"

# Build the OpenMP runtime library using Emscripten (only when OPENMP=1)
$(OPENMP_LIB): $(OPENMP_DIR) $(EMSDK_ENV)
	@echo "Building OpenMP Runtime..."
	@mkdir -p $(BUILD_DIR)
	$(RUN_EM) emcmake cmake \
        -DCMAKE_C_FLAGS="-DPAGESIZE=65536 -pthread -matomics -mbulk-memory" \
        -DCMAKE_CXX_FLAGS="-DPAGESIZE=65536 -pthread -matomics -mbulk-memory" \
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

# Check if OpenMP sources exist (only matters when OPENMP=1 and we try to build libomp)
$(OPENMP_DIR):
	$(error "OpenMP sources not found at $(OPENMP_DIR). Please clone llvm-project.")

clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_DIR)

.PHONY: all omp noomp clean
