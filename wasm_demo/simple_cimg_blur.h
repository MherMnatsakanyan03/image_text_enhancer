#ifndef SIMPLE_CIMG_BLUR_H
#define SIMPLE_CIMG_BLUR_H

#include <cstdint>

// Disable CImg display capabilities for WASM (no X11/Windows GUI)
#define cimg_display 0
// Use our own exception handler
#define cimg_use_cpp11 1

// Tell CImg we're handling OpenMP ourselves (via Emscripten flags)
// This prevents CImg from trying to include <omp.h> directly
#ifdef _OPENMP
#define cimg_use_openmp 1
#else
#define cimg_use_openmp 0
#endif

#include "CImg.h"
using namespace cimg_library;

/**
 * @brief Apply Gaussian blur to an RGBA image in-place.
 *
 * This function takes a raw RGBA buffer, wraps it in a CImg object,
 * applies Gaussian blur, and writes the result back to the buffer.
 *
 * @param rgba_data Pointer to RGBA pixel data (R, G, B, A interleaved).
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param sigma Standard deviation of the Gaussian kernel (default: 2.0).
 */
void apply_blur(uint8_t* rgba_data, int width, int height, float sigma = 2.0f);

/**
 * @brief Get the number of OpenMP threads available.
 *
 * @return Number of threads, or 1 if OpenMP is not available.
 */
int get_thread_count();

#endif // SIMPLE_CIMG_BLUR_H

