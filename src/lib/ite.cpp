/**
 * @file ite.cpp
 * @brief Facade implementation for the Image Text Enhancement (ITE) library.
 *
 * This file provides backward-compatible wrappers that delegate to the
 * specialized module implementations in the subdirectories.
 */

#include "ite.h"

#include "binarization/binarization.h"
#include "color/color.h"

#include "color/grayscale.h"
#include "filters/filters.h"
#include "geometry/geometry.h"
#include "io/image_io.h"
#include "morphology/morphology.h"

#include <chrono> // Added for high-resolution timing
#include <iostream> // Added for logging output

#include "color/contrast.h"

namespace ite
{
    namespace
    {
        void record_time(TimingLog* log, const std::string &name, long long us, bool verbose = false)
        {
            if (log)
            {
                log->push_back({name, us});
            }

            if (verbose)
            {
                std::cout << "[ITE] " << name + ":\t" << us << " us" << std::endl;
            }
        }
    } // namespace

    // ============================================================================
    // I/O Operations
    // ============================================================================

    CImg<uint> loadimage(const std::string &filepath) { return io::load_image(filepath); }

    CImg<uint> writeimage(const CImg<uint> &image, const std::string &filepath) { return io::save_image(image, filepath); }

    // ============================================================================
    // Color Operations
    // ============================================================================

    CImg<uint> to_grayscale(const CImg<uint> &input_image)
    {
        CImg<uint> result = input_image;
        CImg<uint> working(input_image.width(), input_image.height(), input_image.depth(), 1);
        color::to_grayscale_rec601(result, working);
        return working;
    }

    CImg<uint> contrast_enhancement(const CImg<uint> &input_image)
    {
        CImg<uint> result = input_image;
        color::contrast_linear_stretch(result);
        return result;
    }

    CImg<uint> color_pass(const CImg<uint> &bin_image, const CImg<uint> &color_image)
    {
        CImg<uint> result = color_image;
        color::color_pass_inplace(result, bin_image);
        return result;
    }

    // ============================================================================
    // Binarization
    // ============================================================================

    CImg<uint> binarize_sauvola(const CImg<uint> &input_image, int window_size, float k, float delta)
    {
        const int w = input_image.width();
        const int h = input_image.height();
        const int d = input_image.depth();

        CImg<uint> grayscale(w, h, d, 1);
        CImg<uint> binary(w, h, d, 1);

        // Convert to grayscale if needed
        if (input_image.spectrum() == 1)
        {
            grayscale = input_image;
        }
        else
        {
            color::to_grayscale_rec601(input_image, grayscale);
        }

        // Binarize
        binarization::binarize_sauvola(grayscale, binary, window_size, k, delta);
        return binary;
    }

    CImg<uint> binarize_otsu(const CImg<uint> &input_image)
    {
        const int w = input_image.width();
        const int h = input_image.height();
        const int d = input_image.depth();

        CImg<uint> grayscale(w, h, d, 1);
        CImg<uint> binary(w, h, d, 1);

        // Convert to grayscale if needed
        if (input_image.spectrum() == 1)
        {
            grayscale = input_image;
        }
        else
        {
            color::to_grayscale_rec601(input_image, grayscale);
        }

        // Binarize
        binarization::binarize_otsu(grayscale, binary);
        return binary;
    }

    CImg<uint> binarize_bataineh(const CImg<uint> &input_image)
    {
        const int w = input_image.width();
        const int h = input_image.height();
        const int d = input_image.depth();

        CImg<uint> grayscale(w, h, d, 1);
        CImg<uint> binary(w, h, d, 1);

        // Convert to grayscale if needed
        if (input_image.spectrum() == 1)
        {
            grayscale = input_image;
        }
        else
        {
            color::to_grayscale_rec601(input_image, grayscale);
        }

        // Binarize
        binarization::binarize_bataineh(grayscale, binary);
        return binary;
    }

    // ============================================================================
    // Morphological Operations
    // ============================================================================

    CImg<uint> dilation(const CImg<uint> &input_image, int kernel_size)
    {
        CImg<uint> result = input_image;
        CImg<uint> working(input_image.width(), input_image.height(), input_image.depth(), 1);
        morphology::dilation_square(result, working, kernel_size);
        return working;
    }

    CImg<uint> erosion(const CImg<uint> &input_image, int kernel_size)
    {
        CImg<uint> result = input_image;
        CImg<uint> working(input_image.width(), input_image.height(), input_image.depth(), 1);
        morphology::erosion_square(result, working, kernel_size);
        return working;
    }

    CImg<uint> despeckle(const CImg<uint> &input_image, uint threshold, bool diagonal_connections)
    {
        CImg<uint> result = input_image;
        morphology::despeckle_ccl(result, threshold, diagonal_connections);
        return result;
    }

    // ============================================================================
    // Geometric Transformations
    // ============================================================================

    CImg<uint> deskew(const CImg<uint> &input_image, int boundary_conditions)
    {
        CImg<uint> result = input_image;
        geometry::deskew_projection_profile(result, boundary_conditions);
        return result;
    }

    // ============================================================================
    // Filters / Denoising
    // ============================================================================

    CImg<uint> simple_gaussian_blur(const CImg<uint> &input_image, float sigma, int boundary_conditions)
    {
        CImg<uint> result = input_image;
        filters::simple_gaussian_blur(result, sigma, boundary_conditions);
        return result;
    }

    CImg<uint> adaptive_gaussian_blur(const CImg<uint> &input_image, float sigma_low, float sigma_high, float edge_thresh, int /*truncate*/, int block_h)
    {
        CImg<uint> result = input_image;
        CImg<uint> working(input_image.width(), input_image.height(), input_image.depth(), input_image.spectrum());
        filters::adaptive_gaussian_blur(result, working, sigma_low, sigma_high, edge_thresh, block_h);
        return working;
    }

    CImg<uint> simple_median_filter(const CImg<uint> &input_image, int kernel_size, unsigned int threshold)
    {
        CImg<uint> result = input_image;
        filters::simple_median_blur(result, kernel_size, threshold);
        return result;
    }

    CImg<uint> adaptive_median_filter(const CImg<uint> &input_image, int max_window_size, int block_h)
    {
        CImg<uint> result = input_image;
        filters::adaptive_median_filter(result, max_window_size, block_h);
        return result;
    }

    // ============================================================================
    // Full Enhancement Pipeline
    // ============================================================================

    CImg<uint> enhance(const CImg<uint> &input_image, const EnhanceOptions &opt, const int block_h, TimingLog* log, bool verbose)
    {
        using Clock = std::chrono::steady_clock;
        using Us = std::chrono::microseconds;

        auto total_start = Clock::now();
        auto step_start = total_start;

        // Three image copies are always maintained throughout the pipeline:
        //   result      — the primary working image (grayscale, processed in stages)
        //   working     — scratch buffer for functions that need a separate output image (always single-channel, matching result dimensions)
        //   color_image — color copy of the input, always initialized for the color pass step
        CImg<uint> result = input_image;
        CImg<uint> color_image = input_image;

        auto now = Clock::now();
        record_time(log, "Init & Copy", std::chrono::duration_cast<Us>(now - step_start).count(), verbose);
        step_start = now;

        // 2. Grayscale — reads result, writes into working; swap so result holds grayscale
        // Pre-allocate working as single-channel buffer matching result dimensions
        CImg<uint> working(input_image.width(), input_image.height(), input_image.depth(), 1);
        color::to_grayscale_rec601(result, working);
        result.swap(working);
        now = Clock::now();
        record_time(log, "Grayscale", std::chrono::duration_cast<Us>(now - step_start).count(), verbose);
        step_start = now;

        // 3. Deskew — apply_deskew is in-place (rotation); color_image is always deskewed too
        // After deskew, result dimensions may change, so we must resize working to match
        if (opt.do_deskew)
        {
            double angle = geometry::detect_skew_angle(result);
            if (std::abs(angle) > 0.05)
            {
                geometry::apply_deskew(result, angle, opt.boundary_conditions);
                geometry::apply_deskew(color_image, angle, opt.boundary_conditions);
                // Resize working to match result's new dimensions
                working.assign(result.width(), result.height(), result.depth(), 1);
            }
            now = Clock::now();
            record_time(log, "Deskew", std::chrono::duration_cast<Us>(now - step_start).count(), verbose);
            step_start = now;
        }

        // 4. Contrast — in-place on result (no second buffer needed)
        color::contrast_linear_stretch(result);
        now = Clock::now();
        record_time(log, "Contrast", std::chrono::duration_cast<Us>(now - step_start).count(), verbose);
        step_start = now;

        // 5. Denoising — adaptive blur reads result, writes into working; swap so result is updated
        if (opt.do_adaptive_gaussian_blur)
        {
            filters::adaptive_gaussian_blur(result, working, opt.adaptive_sigma_low, opt.adaptive_sigma_high, opt.adaptive_edge_thresh, block_h,
                                            opt.boundary_conditions);
            result.swap(working);
            now = Clock::now();
            record_time(log, "Adaptive Gaussian", std::chrono::duration_cast<Us>(now - step_start).count(), verbose);
            step_start = now;
        }
        else if (opt.do_gaussian_blur)
        {
            // simple_gaussian_blur is in-place — no second buffer needed
            filters::simple_gaussian_blur(result, opt.sigma, opt.boundary_conditions);
            now = Clock::now();
            record_time(log, "Gaussian Blur", std::chrono::duration_cast<Us>(now - step_start).count(), verbose);
            step_start = now;
        }

        if (opt.do_median_blur)
        {
            // simple_median_blur is in-place — no second buffer needed
            filters::simple_median_blur(result, opt.median_kernel_size, opt.median_threshold);
            now = Clock::now();
            record_time(log, "Median Blur", std::chrono::duration_cast<Us>(now - step_start).count(), verbose);
            step_start = now;
        }

        if (opt.do_adaptive_median)
        {
            // adaptive_median_filter is in-place — no second buffer needed
            filters::adaptive_median_filter(result, opt.adaptive_median_max_window, block_h);
            now = Clock::now();
            record_time(log, "Adaptive Median", std::chrono::duration_cast<Us>(now - step_start).count(), verbose);
            step_start = now;
        }

        // 6. Binarization — reads result, writes into working; swap so result holds binary image
        switch (opt.binarization_method)
        {
        case BinarizationMethod::Otsu:
            binarization::binarize_otsu(result, working);
            result.swap(working);
            now = Clock::now();
            record_time(log, "Binarization (Otsu)", std::chrono::duration_cast<Us>(now - step_start).count(), verbose);
            break;
        case BinarizationMethod::Sauvola:
            binarization::binarize_sauvola(result, working, opt.sauvola_window_size, opt.sauvola_k, opt.sauvola_delta);
            result.swap(working);
            now = Clock::now();
            record_time(log, "Binarization (Sauvola)", std::chrono::duration_cast<Us>(now - step_start).count(), verbose);
            break;
        case BinarizationMethod::Bataineh:
            binarization::binarize_bataineh(result, working);
            result.swap(working);
            now = Clock::now();
            record_time(log, "Binarization (Bataineh)", std::chrono::duration_cast<Us>(now - step_start).count(), verbose);
            break;
        }
        step_start = now;

        // 7. Morphology — despeckle is in-place; dilation/erosion read result, write working, then swap
        if (opt.do_despeckle)
        {
            morphology::despeckle_ccl(result, static_cast<uint>(opt.despeckle_threshold), opt.diagonal_connections);
            now = Clock::now();
            record_time(log, "Despeckle", std::chrono::duration_cast<Us>(now - step_start).count(), verbose);
            step_start = now;
        }

        if (opt.do_dilation)
        {
            morphology::dilation_square(result, working, opt.kernel_size);
            result.swap(working);
            now = Clock::now();
            record_time(log, "Dilation", std::chrono::duration_cast<Us>(now - step_start).count(), verbose);
            step_start = now;
        }

        if (opt.do_erosion)
        {
            morphology::erosion_square(result, working, opt.kernel_size);
            result.swap(working);
            now = Clock::now();
            record_time(log, "Erosion", std::chrono::duration_cast<Us>(now - step_start).count(), verbose);
            step_start = now;
        }

        // 8. Color Pass — apply binary result as mask onto color_image
        if (opt.do_color_pass)
        {
            color::color_pass_inplace(color_image, result);
            now = Clock::now();
            record_time(log, "Color Pass", std::chrono::duration_cast<Us>(now - step_start).count(), verbose);

            auto total_time = std::chrono::duration_cast<Us>(now - total_start).count();
            record_time(log, "TOTAL", total_time, verbose);
            return color_image;
        }

        now = Clock::now();
        auto total_time = std::chrono::duration_cast<Us>(now - total_start).count();
        record_time(log, "TOTAL", total_time, verbose);

        return result;
    }

} // namespace ite
