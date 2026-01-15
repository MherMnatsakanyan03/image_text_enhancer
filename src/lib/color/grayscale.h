#pragma once
/**
 * @file grayscale.h
 * @brief Grayscale conversion operations.
 */

#include "CImg.h"

using namespace cimg_library;

namespace ite::color
{
    // Standard luminance weights (Rec. 601)
    constexpr float WEIGHT_R = 0.299f;
    constexpr float WEIGHT_G = 0.587f;
    constexpr float WEIGHT_B = 0.114f;

    // Rec. 709 luminance weights (alternative, used in HD video)
    constexpr float WEIGHT_R_709 = 0.2126f;
    constexpr float WEIGHT_G_709 = 0.7152f;
    constexpr float WEIGHT_B_709 = 0.0722f;

    /**
     * @brief Converts an image to grayscale in-place.
     *
     * Uses the standard luminance formula (Rec. 601):
     * Y = 0.299*R + 0.587*G + 0.114*B
     *
     * If the image is already 1-channel, no conversion is performed.
     *
     * @param image The image to convert (modified in-place).
     */
    void to_grayscale_rec601(CImg<uint> &image);

} // namespace ite::color
