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
     * @brief Detects the skew angle of the text using a fast projection profile method.
     * Uses Sauvola binarization and coordinate projection (Radon transform)
     */
    double detect_skew_angle(const CImg<uint> &input_image, const int window_size = 15, const float k = 0.2, const float delta = 0.0);

    /**
     * @brief Rotates the image by the specified angle.
     */
    void apply_deskew(CImg<uint> &input_image, double angle, int boundary_conditions);


    /**
     * @brief Detects and corrects skew (rotation) in an image (in-place).
     *
     * Uses the Projection Profile method:
     * 1. Converts to grayscale and binarizes using Sauvola
     * 2. Searches for the angle that maximizes horizontal projection profile variance
     * 3. Rotates the image to correct the skew
     *
     * Features:
     * - Polarity-safe (detects light vs. dark background)
     * - Coarse-to-fine angle search for efficiency
     * - Uses Neumann boundary conditions to avoid black corners
     *
     * @param input_image The image to deskew (modified in-place).
     * @param boundary_conditions The boundary condition for rotation (default: 1 = Neumann).
     * @param window_size The size of the local window for Sauvola (default: 15).
     * @param k Sauvola's parameter controlling threshold sensitivity (default: 0.2).
     * @param delta Optional offset subtracted from threshold (default: 0.0).
     */
    void deskew_projection_profile(CImg<uint> &input_image, int boundary_conditions = 1, int window_size = 15, float k = 0.2f, float delta = 0.0f);

} // namespace ite::geometry
