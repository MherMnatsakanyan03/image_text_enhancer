#pragma once
/**
 * @file filters.h
 * @brief Image filtering operations (Gaussian blur, denoising).
 */

#include "CImg.h"

using namespace cimg_library;

namespace ite::filters
{

    /**
     * @brief Applies Gaussian blur to an image in-place.
     *
     * Uses CImg's built-in blur function with Neumann boundary conditions.
     *
     * @param image The image to blur (modified in-place).
     * @param sigma The standard deviation of the Gaussian kernel.
     * @param boundary_conditions The type of boundary conditions to use. See `CImg::blur()` in `CImg.h` for more info (default: 1 = Neumann).
     */
    void simple_gaussian_blur(CImg<uint> &image, float sigma = 1.0f, int boundary_conditions = 1);


    /**
     * @brief Applies an adaptive Gaussian blur to an image in-place using OpenMP.
     *
     * This function applies a Gaussian blur with a variable standard deviation.
     * It blurs less around edges (high variance) and more in flat regions (low variance)
     * to preserve sharpness while reducing noise. The image is processed in parallel
     * in horizontal blocks.
     *
     * @param img The image to blur (modified in-place).
     * @param sigma_low The standard deviation of the kernel for edge regions.
     * @param sigma_high The standard deviation of the kernel for flat regions.
     * @param edge_thresh The variance threshold to differentiate between edge and flat regions.
     * @param block_h Height of the blocks for parallel processing (default: 64).
     */
    void adaptive_gaussian_blur(CImg<uint> &img, float sigma_low, float sigma_high, float edge_thresh, int block_h = 64, int boundary_conditions = 1);

    /**
     * @brief Applies median denoising to an image in-place.
     * @param image The image to blur (modified in-place).
     * @param kernel_size The size of the median filter kernel.
     */
    void simple_median_blur(CImg<uint> &image, int kernel_size = 3, unsigned int threshold = 0);

    /**
     * @brief Applies adaptive median filtering to an image in-place using OpenMP.
     *
     * The adaptive median filter is excellent for removing impulse noise (salt-and-pepper)
     * while preserving edges and fine details. It starts with a 3x3 window and expands
     * up to max_window_size when detecting impulse noise, leaving non-impulse pixels unchanged.
     * This makes it ideal for scan speckle removal in text images.
     *
     * @param img The image to filter (modified in-place).
     * @param max_window_size Maximum window size (must be odd, >= 3, typical: 5, 7, or 9).
     * @param block_h Height of the blocks for parallel processing (default: 64).
     */
    void adaptive_median_filter(CImg<uint> &img, int max_window_size = 7, int block_h = 64);

    /**
     * @brief Parameters for adaptive Gaussian blur.
     *
     * Contains the sigma values and edge threshold for adaptive Gaussian blurring.
     */
    struct AdaptiveGaussianParams
    {
        float sigma_low;    ///< Standard deviation for edge regions (preserves details)
        float sigma_high;   ///< Standard deviation for flat regions (smooths noise)
        float edge_thresh;  ///< Gradient threshold for edge detection
    };

    /**
     * @brief Automatically chooses adaptive Gaussian blur parameters for text enhancement.
     *
     * Analyzes the input grayscale image to estimate noise and edge characteristics,
     * then computes optimal sigma_low, sigma_high, and edge_thresh values for
     * text enhancement. The heuristics are designed to:
     *   - Keep sigma_low small to preserve stroke edges
     *   - Increase sigma_high with noise to smooth background/texture
     *   - Set edge_thresh from gradient distribution so edges stay sharp
     *
     * @param gray A grayscale (1-channel) image to analyze.
     * @return AdaptiveGaussianParams struct with optimal parameters.
     */
    AdaptiveGaussianParams choose_sigmas_for_text_enhancement(const CImg<uint> &gray);

} // namespace ite::filters
