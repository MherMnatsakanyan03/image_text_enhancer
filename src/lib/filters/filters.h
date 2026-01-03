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
     */
    void gaussian_blur(CImg<uint> &image, float sigma = 1.0f);

} // namespace ite::filters
