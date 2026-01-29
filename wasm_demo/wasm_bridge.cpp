#include "../src/lib/ite.h"
#include <emscripten.h>
#include <iostream>
#include <vector>
#include <algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

extern "C" {

struct WasmEnhanceOptions {
    int boundary_conditions;
    int do_gaussian_blur;
    int do_median_blur;
    int do_adaptive_median;
    int do_adaptive_gaussian_blur;
    int do_color_pass;
    float sigma;
    float adaptive_sigma_low;
    float adaptive_sigma_high;
    float adaptive_edge_thresh;
    int median_kernel_size;
    float median_threshold;
    int adaptive_median_max_window;
    int diagonal_connections;
    int do_erosion;
    int do_dilation;
    int do_despeckle;
    int kernel_size;
    int despeckle_threshold;
    int do_deskew;
    int sauvola_window_size;
    float sauvola_k;
    float sauvola_delta;
};

} // extern "C"

ite::EnhanceOptions to_cpp_options(const WasmEnhanceOptions* opts) {
    ite::EnhanceOptions cpp_opts;
    if (opts) {
        cpp_opts.boundary_conditions = opts->boundary_conditions;
        cpp_opts.do_gaussian_blur = (bool)opts->do_gaussian_blur;
        cpp_opts.do_median_blur = (bool)opts->do_median_blur;
        cpp_opts.do_adaptive_median = (bool)opts->do_adaptive_median;
        cpp_opts.do_adaptive_gaussian_blur = (bool)opts->do_adaptive_gaussian_blur;
        cpp_opts.do_color_pass = (bool)opts->do_color_pass;
        cpp_opts.sigma = opts->sigma;
        cpp_opts.adaptive_sigma_low = opts->adaptive_sigma_low;
        cpp_opts.adaptive_sigma_high = opts->adaptive_sigma_high;
        cpp_opts.adaptive_edge_thresh = opts->adaptive_edge_thresh;
        cpp_opts.median_kernel_size = opts->median_kernel_size;
        cpp_opts.median_threshold = opts->median_threshold;
        cpp_opts.adaptive_median_max_window = opts->adaptive_median_max_window;
        cpp_opts.diagonal_connections = (bool)opts->diagonal_connections;
        cpp_opts.do_erosion = (bool)opts->do_erosion;
        cpp_opts.do_dilation = (bool)opts->do_dilation;
        cpp_opts.do_despeckle = (bool)opts->do_despeckle;
        cpp_opts.kernel_size = opts->kernel_size;
        cpp_opts.despeckle_threshold = opts->despeckle_threshold;
        cpp_opts.do_deskew = (bool)opts->do_deskew;
        cpp_opts.sauvola_window_size = opts->sauvola_window_size;
        cpp_opts.sauvola_k = opts->sauvola_k;
        cpp_opts.sauvola_delta = opts->sauvola_delta;
    }
    return cpp_opts;
}

extern "C" {

EMSCRIPTEN_KEEPALIVE
void process_image_with_options(uint8_t* rgba_data, int width, int height, WasmEnhanceOptions* opts) {

    std::cout << "[WASM] process_image_with_options called: " << width << "x" << height << std::endl;

    if (!rgba_data || width <= 0 || height <= 0) {
        std::cerr << "[WASM] Error: invalid inputs" << std::endl;
        return;
    }

    try {
        // Convert interleaved RGBA to CImg Planar
        cimg_library::CImg<unsigned int> img(width, height, 1, 4);

        std::cout << "[WASM] Reading input data..." << std::endl;
        #pragma omp parallel for
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 4;
                img(x, y, 0, 0) = rgba_data[idx + 0]; // R
                img(x, y, 0, 1) = rgba_data[idx + 1]; // G
                img(x, y, 0, 2) = rgba_data[idx + 2]; // B
                img(x, y, 0, 3) = rgba_data[idx + 3]; // A
            }
        }

        ite::EnhanceOptions cpp_opts = to_cpp_options(opts);

        std::cout << "[WASM] Calling ite::enhance..." << std::endl;
        cimg_library::CImg<unsigned int> result = ite::enhance(img, cpp_opts);
        
        if (result.width() != width || result.height() != height) {
             std::cerr << "[WASM] Warning: Result dimensions differ. Resizing to fit original buffer." << std::endl;
             result.resize(width, height);
        }

        // Write back
        std::cout << "[WASM] Writing output data..." << std::endl;
        #pragma omp parallel for
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 4;
                int spectrum = result.spectrum();
                
                if (spectrum == 1) {
                    // Grayscale/Binary
                    unsigned int val = result(x, y, 0, 0);
                    uint8_t v = val > 255 ? 255 : (uint8_t)val;
                    rgba_data[idx + 0] = v;
                    rgba_data[idx + 1] = v;
                    rgba_data[idx + 2] = v;
                    rgba_data[idx + 3] = 255;
                } else {
                     // Color (assumed 3+ channels)
                    rgba_data[idx + 0] = (uint8_t)std::min(result(x, y, 0, 0), 255u);
                    rgba_data[idx + 1] = (uint8_t)std::min(result(x, y, 0, 1), 255u);
                    rgba_data[idx + 2] = (uint8_t)std::min(result(x, y, 0, 2), 255u);
                    rgba_data[idx + 3] = 255; 
                }
            }
        }
        
        std::cout << "[WASM] Processing complete." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[WASM] Exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[WASM] Unknown exception" << std::endl;
    }
}

EMSCRIPTEN_KEEPALIVE
void process_image(uint8_t* rgba_data, int width, int height) {
    process_image_with_options(rgba_data, width, height, nullptr);
}

EMSCRIPTEN_KEEPALIVE
int wasm_get_thread_count() {
#ifdef _OPENMP
    return omp_get_max_threads();
#else
    return 1;
#endif
}

} // extern "C"

