#pragma once
/**
 * @file integral_image.h
 * @brief Integral image (summed-area table) utilities for fast region sum calculations.
 */

#include "CImg.h"

using namespace cimg_library;

namespace ite::core
{

    /**
     * @brief Computes the Summed-Area Table (Integral Image).
     *
     * The integral image allows constant-time calculation of the sum of
     * pixel values in any rectangular region.
     *
     * @param src Source image
     * @return Integral image as CImg<double>
     */
    CImg<double> calculate_integral_image(const CImg<double> &src);

    /**
     * @brief Computes the sum of a rectangular region from an integral image.
     *
     * Uses the inclusion-exclusion principle for O(1) computation.
     *
     * @param integral_img The precomputed integral image
     * @param x1 Left boundary (inclusive)
     * @param y1 Top boundary (inclusive)
     * @param z Depth slice
     * @param c Channel
     * @param x2 Right boundary (inclusive)
     * @param y2 Bottom boundary (inclusive)
     * @return Sum of pixel values in the specified region
     */
    double get_area_sum(const CImg<double> &integral_img, int x1, int y1, int z, int c, int x2, int y2);

} // namespace ite::core
