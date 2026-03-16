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
     * @brief Binarizes a grayscale image using Sauvola's method, writing the result into output.
     *
     * Sauvola's adaptive thresholding considers local mean and standard deviation
     * to compute a threshold for each pixel, making it robust against
     * uneven illumination.
     *
     * @param input       The grayscale source image (read-only).
     * @param output      The destination image (overwritten with 0/255 binary values).
     * @param window_size The size of the local window (default: 15).
     * @param k           Sauvola's parameter controlling threshold sensitivity (default: 0.2).
     * @param delta       Optional offset subtracted from threshold (default: 0.0).
     * @throws std::runtime_error if the input is not grayscale (1-channel).
     */
    void binarize_sauvola(const CImg<uint> &input, CImg<uint> &output, int window_size = 15, float k = 0.2f, float delta = 0.0f);

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

    /**
     * @brief Binarizes a grayscale image using Otsu's method, writing the result into output.
     *
     * Uses Otsu's threshold to separate foreground from background.
     * The border mean is used to determine if the background is light or dark,
     * and adjust the binarization accordingly (dark text on light background vs. light text on dark background).
     *
     * @param input  The grayscale source image (read-only).
     * @param output The destination image (overwritten with 0/255 binary values).
     * @throws std::runtime_error if the input is not grayscale (1-channel).
     */
    void binarize_otsu(const CImg<uint> &input, CImg<uint> &output);

    /**
     * @brief Binarizes a grayscale image using Bataineh's method, writing the result into output.
     *
     * Uses Bataineh's adaptive threshold to separate foreground from background.
     * The method adapts window sizes based on image characteristics for improved results.
     * Uses adaptive local thresholds computed from local mean and standard deviation for separation.
     *
     * @param input  The grayscale source image (read-only).
     * @param output The destination image (overwritten with 0/255 binary values).
     * @throws std::runtime_error if the input is not grayscale (1-channel).
     */
    void binarize_bataineh(const CImg<uint> &input, CImg<uint> &output);

} // namespace ite::binarization
