#pragma once
/**
 * @file binarization.h
 * @brief Image binarization algorithms (Sauvola, Otsu).
 */

#include "CImg.h"

using namespace cimg_library;

namespace ite::binarization
{

    /**
     * @brief Binarizes a grayscale image in-place using Sauvola's method.
     *
     * Sauvola's adaptive thresholding considers local mean and standard deviation
     * to compute a threshold for each pixel, making it robust against
     * uneven illumination.
     *
     * @param image The grayscale image to binarize (modified in-place).
     * @param window_size The size of the local window (default: 15).
     * @param k Sauvola's parameter controlling threshold sensitivity (default: 0.2).
     * @param delta Optional offset subtracted from threshold (default: 0.0).
     * @throws std::runtime_error if the image is not grayscale (1-channel).
     */
    void binarize_sauvola(CImg<uint> &image, int window_size = 15, float k = 0.2f, float delta = 0.0f);

    /**
     * @brief Computes Otsu's threshold for a grayscale image.
     *
     * Otsu's method finds the threshold that minimizes intra-class variance
     * (or equivalently maximizes inter-class variance).
     *
     * @param gray The grayscale image (8-bit).
     * @return The optimal threshold value (0-255).
     */
    int compute_otsu_threshold(const CImg<unsigned char> &gray);

    /**
     * @brief Computes the mean intensity of border pixels.
     *
     * Used to detect whether the background is light or dark.
     *
     * @param gray The grayscale image.
     * @return The average intensity of border pixels.
     */
    double compute_border_mean(const CImg<unsigned char> &gray);

} // namespace ite::binarization
