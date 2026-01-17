#pragma once
/**
 * @file color.h
 * @brief Color operations.
 */

#include "CImg.h"

using namespace cimg_library;

namespace ite::color
{

    /**
     * @brief Passes color through a binary image mask. The color image will be overwritten in place.
     *
     * @param color_image The color image to be passed through the mask.
     * @param bin_image The binary mask image.
     */
    void color_pass_inplace(CImg<uint> &color_image, const CImg<uint> &bin_image);

} // namespace ite::color
