// Optimized version with separable convolution
// This file demonstrates the optimization approach for simple_cimg_blur.cpp

#include "simple_cimg_blur.h"
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten/threading.h>
#endif

void apply_blur_optimized(uint8_t* rgba_data, int width, int height, float sigma) {
    std::cout << "[C++] apply_blur_optimized: " << width << "x" << height
              << ", sigma=" << sigma << std::endl;

    try {
        // Create CImg with planar layout (RRRR...GGGG...BBBB...AAAA)
        CImg<uint8_t> img(width, height, 1, 4);

        // Copy from interleaved RGBA to planar - optimized with cache-friendly access
        for (int c = 0; c < 4; ++c) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    int idx = (y * width + x) * 4;
                    img(x, y, 0, c) = rgba_data[idx + c];
                }
            }
        }

        std::cout << "[C++] Data copied to CImg (planar format)" << std::endl;

        // OPTIMIZATION: Use separable convolution instead of 2D blur
        // Gaussian blur is separable: 2D filter = 1D horizontal * 1D vertical
        // This reduces complexity from O(n²) to O(2n) for kernel size n
        // Speedup: 3-5× for typical sigma values

        CImgList<uint8_t> channels = img.get_split('c');

        // Blur R, G, B channels (0, 1, 2) using separable approach
        for (int c = 0; c < 3; ++c) {
            // Horizontal pass (blur along x-axis)
            channels[c].blur_x(sigma, 1, true);  // Neumann boundary, Gaussian

            // Vertical pass (blur along y-axis)
            channels[c].blur_y(sigma, 1, true);  // Neumann boundary, Gaussian
        }
        // Keep alpha channel unchanged (channels[3])

        img = channels.get_append('c');

        std::cout << "[C++] Separable Gaussian blur applied (sigma=" << sigma << ")" << std::endl;

        // Copy back from planar to interleaved RGBA - optimized
        for (int c = 0; c < 4; ++c) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    int idx = (y * width + x) * 4;
                    rgba_data[idx + c] = img(x, y, 0, c);
                }
            }
        }

        std::cout << "[C++] Data copied back to RGBA buffer" << std::endl;

    } catch (const CImgException& e) {
        std::cerr << "[C++] CImg error: " << e.what() << std::endl;
        throw;
    } catch (const std::exception& e) {
        std::cerr << "[C++] Error: " << e.what() << std::endl;
        throw;
    }
}

// Alternative: Even more optimized version that avoids some copies
void apply_blur_inplace(uint8_t* rgba_data, int width, int height, float sigma) {
    std::cout << "[C++] apply_blur_inplace: " << width << "x" << height << std::endl;

    try {
        // Process each RGB channel separately
        std::vector<float> temp(width * height);

        for (int c = 0; c < 3; ++c) {  // R, G, B only
            // Extract channel to temporary buffer
            for (int i = 0; i < width * height; ++i) {
                temp[i] = static_cast<float>(rgba_data[i * 4 + c]);
            }

            // Create CImg view of the temporary buffer (no copy)
            CImg<float> channel(temp.data(), width, height, 1, 1, true);

            // Apply separable blur
            channel.blur_x(sigma, 1, true);
            channel.blur_y(sigma, 1, true);

            // Write back to RGBA buffer
            for (int i = 0; i < width * height; ++i) {
                rgba_data[i * 4 + c] = static_cast<uint8_t>(
                    std::min(255.0f, std::max(0.0f, temp[i]))
                );
            }
        }
        // Alpha channel unchanged

        std::cout << "[C++] In-place blur complete" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[C++] Error: " << e.what() << std::endl;
        throw;
    }
}

/*
 * PERFORMANCE COMPARISON (estimated for 6720×4480 image):
 *
 * Original (2D blur):           ~2290ms
 * Optimized (separable):        ~600ms   (3.8× faster)
 * In-place (separable):         ~500ms   (4.6× faster)
 * With SIMD (separable+SIMD):   ~400ms   (5.7× faster)
 *
 * IMPLEMENTATION NOTES:
 *
 * 1. Separable convolution:
 *    - 2D Gaussian = horizontal 1D * vertical 1D
 *    - Reduces kernel operations from O(w²) to O(2w)
 *    - Example: 5×5 kernel = 25 ops → 5+5 = 10 ops per pixel
 *    - This is the BIGGEST win (3-5× speedup)
 *
 * 2. Memory layout optimization:
 *    - Process by channel reduces cache misses
 *    - Loop order matters: inner loop should be contiguous memory
 *
 * 3. SIMD (from build flags):
 *    - -msimd128 enables WebAssembly SIMD
 *    - -msse/-msse2 hints to CImg to use SIMD operations
 *    - Gives additional 20-30% improvement
 *
 * 4. Avoid unnecessary copies:
 *    - Original code: RGBA → planar → blur → planar → RGBA
 *    - Optimized: Extract channel → blur → write back
 *    - Saves 2 full-image copies
 *
 * NEXT STEPS:
 *
 * 1. Replace apply_blur() in simple_cimg_blur.cpp with apply_blur_optimized()
 * 2. Rebuild with ./build.sh (includes SIMD flags now)
 * 3. Test in browser, measure new performance
 * 4. If satisfactory, keep this implementation
 * 5. If still too slow, try apply_blur_inplace() or consider WebGPU
 */

