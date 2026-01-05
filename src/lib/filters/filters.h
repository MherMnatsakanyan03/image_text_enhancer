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
    void gaussian_blur(CImg<uint> &image, float sigma = 1.0f, int boundary_conditions = 1);


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
     * @param truncate Factor to determine kernel size from sigma (default: 3).
     * @param block_h Height of the blocks for parallel processing (default: 64).
     */
    void adaptive_gaussian_blur_omp(CImg<uint> &img, float sigma_low, float sigma_high,
                                float edge_thresh,
                                int truncate = 3, int block_h = 64);


    /**
     * @brief Applies median denoising to an image in-place.
     * @param image The image to blur (modified in-place).
     * @param kernel_size The size of the median filter kernel.
     */
void median_blur(CImg<uint> &image, int kernel_size = 3, unsigned int threshold = 0);

} // namespace ite::filters
