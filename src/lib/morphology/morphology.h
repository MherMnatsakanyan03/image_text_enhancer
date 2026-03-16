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
     * @brief Performs morphological dilation.
     *
     * Dilation expands bright (white) regions. On binary images, this
     * can connect broken character parts or thicken strokes.
     *
     * @param input The source image to read from.
     * @param output The caller-provided buffer where the dilated result is written.
     * @param kernel_size The size of the structuring element (e.g., 3 for 3x3).
     * @throws std::runtime_error if the image is not single-channel.
     */
    void dilation_square(const CImg<uint> &input, CImg<uint> &output, int kernel_size = 3);

    /**
     * @brief Performs morphological erosion.
     *
     * Erosion shrinks bright regions (expands dark regions). On binary images,
     * this can remove small noise specks or thin strokes.
     *
     * @param input The source image to read from.
     * @param output The caller-provided buffer where the eroded result is written.
     * @param kernel_size The size of the structuring element (e.g., 3 for 3x3).
     * @throws std::runtime_error if the image is not single-channel.
     */
    void erosion_square(const CImg<uint> &input, CImg<uint> &output, int kernel_size = 3);

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
