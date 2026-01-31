#pragma once
/**
 * @file integral_image.h
 * @brief Integral image (summed-area table) utilities for fast region sum calculations.
 */

#include <vector>
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

    /**
     * @brief Computes fused Sum and Sum-of-Squares integral images into padded vectors.
     * * Optimized for performance:
     * - Single pass over the image.
     * - No CImg allocation (uses std::vector).
     * - Padded (Width+1, Height+1) to eliminate boundary checks during lookup.
     * * @param src Source image (uint).
     * @param z Depth slice to process.
     * @param S Output vector for Sum. Resized automatically.
     * @param S2 Output vector for Sum-of-Squares. Resized automatically.
     */
    void compute_fused_integrals(const CImg<uint> &src, int z, std::vector<double> &S, std::vector<double> &S2);

    /**
     * @brief Fast O(1) sum lookup for padded integral image vectors.
     * * @param S The padded integral data (vector).
     * @param w The width of the ORIGINAL image (stride will be w+1).
     * @param x1 Left (inclusive).
     * @param y1 Top (inclusive).
     * @param x2 Right (inclusive).
     * @param y2 Bottom (inclusive).
     */
    inline double get_sum_padded(const std::vector<double> &S, int w, int x1, int y1, int x2, int y2)
    {
        // Padded mapping:
        // Image pixel (x,y) sum is at index (y+1)*(w+1) + (x+1)
        // Rectangle Sum = D - B - C + A
        const int stride = w + 1;
        const int idx_D = (y2 + 1) * stride + (x2 + 1);
        const int idx_B = (y2 + 1) * stride + x1;
        const int idx_C = y1 * stride + (x2 + 1);
        const int idx_A = y1 * stride + x1;

        return S[idx_D] - S[idx_B] - S[idx_C] + S[idx_A];
    }
} // namespace ite::core
