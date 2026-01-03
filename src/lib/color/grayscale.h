#pragma once
/**
 * @file grayscale.h
 * @brief Grayscale conversion operations.
 */

#include "CImg.h"

using namespace cimg_library;

namespace ite::color
{

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
