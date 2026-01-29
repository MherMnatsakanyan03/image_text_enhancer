#include "simple_cimg_blur.h"
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten/threading.h>
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

void apply_blur(uint8_t* rgba_data, int width, int height, float sigma) {
    std::cout << "[C++] apply_blur called: " << width << "x" << height
              << ", sigma=" << sigma << std::endl;

    try {
        // CImg stores data as planar (RRRR...GGGG...BBBB...AAAA)
        // but input is interleaved (RGBARGBARGBA...)
        // We need to create a CImg and copy the data

        CImg<uint8_t> img(width, height, 1, 4); // 1 slice, 4 channels (RGBA)

        // Copy from interleaved RGBA to planar
        #pragma omp parallel for collapse(2) if(width * height > 10000)
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 4;
                img(x, y, 0, 0) = rgba_data[idx + 0]; // R
                img(x, y, 0, 1) = rgba_data[idx + 1]; // G
                img(x, y, 0, 2) = rgba_data[idx + 2]; // B
                img(x, y, 0, 3) = rgba_data[idx + 3]; // A
            }
        }

        std::cout << "[C++] Data copied to CImg (planar format)" << std::endl;

        // Apply Gaussian blur to RGB channels only (not alpha)
        // CImg::blur(sigma, boundary_condition=1, is_gaussian=true)
        CImgList<uint8_t> channels = img.get_split('c');

        // Blur R, G, B channels (0, 1, 2)
        for (int c = 0; c < 3; ++c) {
            channels[c].blur(sigma, 1, true); // Neumann boundary, Gaussian
        }

        // Keep alpha channel unchanged (channels[3])
        img = channels.get_append('c');

        std::cout << "[C++] Gaussian blur applied (sigma=" << sigma << ")" << std::endl;

        // Copy back from planar to interleaved RGBA
        #pragma omp parallel for collapse(2) if(width * height > 10000)
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 4;
                rgba_data[idx + 0] = img(x, y, 0, 0); // R
                rgba_data[idx + 1] = img(x, y, 0, 1); // G
                rgba_data[idx + 2] = img(x, y, 0, 2); // B
                rgba_data[idx + 3] = img(x, y, 0, 3); // A
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

int get_thread_count() {
#ifdef _OPENMP
    int threads = omp_get_max_threads();
    std::cout << "[C++] OpenMP enabled: " << threads << " threads" << std::endl;
    return threads;
#elif defined(__EMSCRIPTEN_PTHREADS__)
    // Emscripten pthreads - return the configured pool size
    // The pool size is set at compile time with -sPTHREAD_POOL_SIZE
    int threads = emscripten_num_logical_cores();
    std::cout << "[C++] Emscripten pthreads enabled: " << threads << " logical cores" << std::endl;
    return threads;
#else
    std::cout << "[C++] Single-threaded mode" << std::endl;
    return 1;
#endif
}

