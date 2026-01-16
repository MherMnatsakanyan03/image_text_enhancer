#pragma once
/**
 * @file contrast.h
 * @brief Contrast enhancement operations.
 */

#include "CImg.h"

using namespace cimg_library;

namespace ite::color
{

    /**
     * @brief Applies robust linear contrast stretching in-place.
     *
     * Clips the bottom 1% and top 1% of intensities to ignore outliers,
     * then stretches the remaining range to 0-255.
     *
     * @param image The image to enhance (modified in-place).
     */
    void contrast_linear_stretch(CImg<uint> &image);

} // namespace ite::color
