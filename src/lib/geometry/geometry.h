#pragma once
/**
 * @file geometry.h
 * @brief Geometric transformations (deskew, rotation).
 */

#include "CImg.h"

using namespace cimg_library;

namespace ite::geometry
{

    /**
     * @brief Detects and corrects skew (rotation) in an image (in-place).
     *
     * Uses the Projection Profile method:
     * 1. Converts to grayscale and binarizes
     * 2. Searches for the angle that maximizes horizontal projection profile variance
     * 3. Rotates the image to correct the skew
     *
     * Features:
     * - Polarity-safe (detects light vs. dark background)
     * - Coarse-to-fine angle search for efficiency
     * - Uses Neumann boundary conditions to avoid black corners
     *
     * @param image The image to deskew (modified in-place).
     */
    void deskew_projection_profile(CImg<uint> &input_image, int boundary_conditions = 1);

    /**
     * @brief Detects the skew angle without applying the correction.
     *
     * Useful for diagnostics or when you want to apply the rotation separately.
     *
     * @param image The image to analyze.
     * @return The detected skew angle in degrees. Positive = clockwise.
     */
    double detect_skew_angle_projection_profile(const CImg<uint> &image);

} // namespace ite::geometry
