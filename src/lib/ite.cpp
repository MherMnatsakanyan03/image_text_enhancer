/**
 * @file ite.cpp
 * @brief Facade implementation for the Image Text Enhancement (ITE) library.
 *
 * This file provides backward-compatible wrappers that delegate to the
 * specialized module implementations in the subdirectories.
 */

#include "ite.h"

#include "binarization/binarization.h"
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

    // ============================================================================
    // Binarization
    // ============================================================================

    CImg<uint> binarize(const CImg<uint> &input_image)
    {
        CImg<uint> result = input_image;
        // Ensure grayscale first
        if (result.spectrum() != 1)
        {
            color::to_grayscale_rec601(result);
        }
        binarization::binarize_sauvola(result);
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

    CImg<uint> deskew(const CImg<uint> &input_image)
    {
        CImg<uint> result = input_image;
        geometry::deskew_projection_profile(result);
        return result;
    }

    // ============================================================================
    // Filters / Denoising
    // ============================================================================

    CImg<uint> gaussian_denoise(const CImg<uint> &input_image, float sigma, int boundary_conditions)
    {
        CImg<uint> result = input_image;
        filters::gaussian_blur(result, sigma, boundary_conditions);
        return result;
    }

    // ============================================================================
    // Full Enhancement Pipeline
    // ============================================================================

    CImg<uint> enhance(const CImg<uint> &input_image, float sigma, int kernel_size, int despeckle_threshold, bool diagonal_connections, bool do_erosion,
                       bool do_dilation, bool do_despeckle, bool do_deskew)
    {
        CImg<uint> result = input_image;

        // 1. Convert to grayscale
        color::to_grayscale_rec601(result);

        // 2. Deskew if requested
        if (do_deskew)
        {
            geometry::deskew_projection_profile(result);
        }

        // 3. Contrast enhancement
        color::contrast_linear_stretch(result);

        // 4. Optional denoising (currently disabled in pipeline, but sigma is available)
        (void)sigma; // Reserved for future use
        // filters::gaussian_blur(result, sigma);
        // filters::median_denoise(result, 3);

        // 5. Binarization
        binarization::binarize_sauvola(result, 15, 0.2f, 4.0f);

        // 6. Despeckle if requested
        if (do_despeckle)
        {
            morphology::despeckle_ccl(result, static_cast<uint>(despeckle_threshold), diagonal_connections);
        }

        // 7. Morphological operations
        if (do_dilation)
        {
            morphology::dilation_square(result, kernel_size);
        }

        if (do_erosion)
        {
            morphology::erosion_square(result, kernel_size);
        }

        return result;
    }

} // namespace ite
