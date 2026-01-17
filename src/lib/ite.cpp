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
#include "color/contrast.h"
#include "color/grayscale.h"
#include "filters/filters.h"
#include "geometry/geometry.h"
#include "io/image_io.h"
#include "morphology/morphology.h"

namespace ite
{

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
        color::to_grayscale_rec601(result);
        return result;
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
        CImg<uint> result = input_image;
        // Ensure grayscale first
        if (result.spectrum() != 1)
        {
            color::to_grayscale_rec601(result);
        }
        binarization::binarize_sauvola(result, window_size, k, delta);
        return result;
    }

    CImg<uint> binarize_otsu(const CImg<uint> &input_image)
    {
        CImg<uint> result = input_image;
        // Ensure grayscale first
        if (result.spectrum() != 1)
        {
            color::to_grayscale_rec601(result);
        }
        binarization::binarize_otsu(result);
        return result;
    }

    CImg<uint> binarize_bataineh(const CImg<uint> &input_image)
    {
        CImg<uint> result = input_image;
        // Ensure grayscale first
        if (result.spectrum() != 1)
        {
            color::to_grayscale_rec601(result);
        }
        binarization::binarize_bataineh(result);
        return result;
    }

    // ============================================================================
    // Morphological Operations
    // ============================================================================

    CImg<uint> dilation(const CImg<uint> &input_image, int kernel_size)
    {
        CImg<uint> result = input_image;
        morphology::dilation_square(result, kernel_size);
        return result;
    }

    CImg<uint> erosion(const CImg<uint> &input_image, int kernel_size)
    {
        CImg<uint> result = input_image;
        morphology::erosion_square(result, kernel_size);
        return result;
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

    CImg<uint> adaptive_gaussian_blur(const CImg<uint> &input_image, float sigma_low, float sigma_high, float edge_thresh, int truncate, int block_h)
    {
        CImg<uint> result = input_image;
        filters::adaptive_gaussian_blur(result, sigma_low, sigma_high, edge_thresh, truncate, block_h);
        return result;
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

    CImg<uint> enhance(const CImg<uint> &input_image, const EnhanceOptions &opt)
    {
        CImg<uint> result = input_image;
        CImg<uint> color_image;

        // Preserve color image if color pass is requested
        if (opt.do_color_pass)
        {
            color_image = input_image;
        }

        // 1. Convert to grayscale
        color::to_grayscale_rec601(result);

        // 2. Deskew if requested
        if (opt.do_deskew)
        {
            geometry::deskew_projection_profile(result, opt.boundary_conditions);
            if (opt.do_color_pass)
            {
                geometry::deskew_projection_profile(color_image, opt.boundary_conditions);
            }
        }

        // 3. Contrast enhancement
        color::contrast_linear_stretch(result);

        // 4. Denoising
        if (opt.do_adaptive_gaussian_blur)
        {
            filters::adaptive_gaussian_blur(result, opt.adaptive_sigma_low, opt.adaptive_sigma_high, opt.adaptive_edge_thresh, 8, opt.boundary_conditions);
        }
        else if (opt.do_gaussian_blur)
        {
            filters::simple_gaussian_blur(result, opt.sigma, opt.boundary_conditions);
        }
        if (opt.do_median_blur)
        {
            filters::simple_median_blur(result, opt.median_kernel_size, opt.median_threshold);
        }
        if (opt.do_adaptive_median)
        {
            filters::adaptive_median_filter(result, opt.adaptive_median_max_window);
        }

        // 5. Binarization
        switch (opt.binarization_method)
        {
        case BinarizationMethod::Otsu:
            binarization::binarize_otsu(result);
            break;
        case BinarizationMethod::Sauvola:
            binarization::binarize_sauvola(result, opt.sauvola_window_size, opt.sauvola_k, opt.sauvola_delta);
            break;
        case BinarizationMethod::Bataineh:
            binarization::binarize_bataineh(result);
            break;
        }

        // 6. Despeckle if requested
        if (opt.do_despeckle)
        {
            morphology::despeckle_ccl(result, static_cast<uint>(opt.despeckle_threshold), opt.diagonal_connections);
        }

        // 7. Morphological operations
        if (opt.do_dilation)
        {
            morphology::dilation_square(result, opt.kernel_size);
        }

        if (opt.do_erosion)
        {
            morphology::erosion_square(result, opt.kernel_size);
        }

        // 8. Color pass if requested
        if (opt.do_color_pass)
        {
            color::color_pass_inplace(color_image, result);
            return color_image;
        }

        return result;
    }

} // namespace ite
