#pragma once
/**
 * @file morphology.h
 * @brief Morphological operations (dilation, erosion, despeckle).
 */

#include "CImg.h"

using namespace cimg_library;

namespace ite::morphology
{

    /**
     * @brief Performs morphological dilation in-place.
     *
     * Dilation expands bright (white) regions. On binary images, this
     * can connect broken character parts or thicken strokes.
     *
     * @param image The image to dilate (modified in-place).
     * @param kernel_size The size of the structuring element (e.g., 3 for 3x3).
     * @throws std::runtime_error if the image is not single-channel.
     */
    void dilation_square(CImg<uint> &image, int kernel_size = 3);

    /**
     * @brief Performs morphological erosion in-place.
     *
     * Erosion shrinks bright regions (expands dark regions). On binary images,
     * this can remove small noise specks or thin strokes.
     *
     * @param image The image to erode (modified in-place).
     * @param kernel_size The size of the structuring element (e.g., 3 for 3x3).
     * @throws std::runtime_error if the image is not single-channel.
     */
    void erosion_square(CImg<uint> &image, int kernel_size = 3);

    /**
     * @brief Removes small connected components (speckles) from a binary image in-place.
     *
     * Components smaller than the threshold are removed.
     *
     * @param image The binary image to despeckle (modified in-place).
     * @param threshold Components with fewer pixels than this are removed.
     * @param diagonal_connections If true, 8-connectivity; if false, 4-connectivity.
     */
    void despeckle_ccl(CImg<uint> &image, uint threshold, bool diagonal_connections = true);

} // namespace ite::morphology
