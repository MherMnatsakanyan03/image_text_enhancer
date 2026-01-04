#pragma once
#include <string>
#include "CImg.h"

using namespace cimg_library;

/**
 * @namespace ite
 * @brief Contains functions for Image Text Enhancement (ITE) to improve OCR readability.
 */
namespace ite
{
    /**
     * @brief Loads an image from a specified file path.
     * @param filepath The relative or absolute path to the image file.
     * @return A CImg<uint> object containing the image data.
     * @throws CImgIOException if the file cannot be opened or recognized.
     */
    CImg<uint> loadimage(const std::string &filepath);

    /**
     * @brief Saves an image to a specified file path.
     * @param image The CImg<uint> object containing the image data to save.
     * @param filepath The relative or absolute path where the image will be saved.
     * @throws CImgIOException if the file cannot be written.
     */
    CImg<uint> writeimage(const CImg<uint> &image, const std::string &filepath);

    /**
     * @brief Converts an image to grayscale.
     * If the image is already 1-channel, a copy is returned.
     * @param input_image The source image (can be 1, 3, or 4 channels).
     * @return A new 1-channel (grayscale) image.
     */
    CImg<uint> to_grayscale(const CImg<uint> &input_image);

    /**
     * @brief Converts a grayscale image to a binary (black and white) image.
     * If the image is not grayscale, it is first converted to grayscale.
     * Uses Sauvola's method for adaptive thresholding.
     * @param input_image The source grayscale image.
     * @return A new 1-channel binary image (values 0 or 255).
     */
    CImg<uint> binarize(const CImg<uint> &input_image);

    /**
     * @brief Applies a Gaussian blur to denoise the image.
     * Best used on a grayscale image before binarization.
     * @param input_image The source image.
     * @param sigma The standard deviation of the Gaussian kernel. Larger values mean more blur.
     * * @param boundary_conditions The type of boundary conditions to use. See `CImg::blur()` in `CImg.h` for more info (default: 1 = Neumann).
     * @return A new, blurred image.
     */
    CImg<uint> gaussian_denoise(const CImg<uint> &input_image, float sigma = 1.0f, int boundary_conditions = 1);

    /**
     * @brief Performs morphological dilation on the image.
     * This operation thickens bright regions. On a binary image, it connects broken character parts.
     * @param input_image The source image (typically binary).
     * @param kernel_size The size of the structuring element (e.g., 3 for a 3x3 kernel).
     * @return A new, dilated image.
     */
    CImg<uint> dilation(const CImg<uint> &input_image, int kernel_size = 3);

    /**
     * @brief Performs morphological erosion on the image.
     * This operation thickens dark regions. On a binary image, it removes small noise specks.
     * @param input_image The source image (typically binary).
     * @param kernel_size The size of the structuring element (e.g., 3 for a 3x3 kernel).
     * @return A new, eroded image.
     */
    CImg<uint> erosion(const CImg<uint> &input_image, int kernel_size = 3);

    /**
     * @brief Detects and corrects for skew (rotation) in the image.
     * Finds the dominant text angle and rotates the image to be level.
     * @param input_image The source image (typically grayscale).
     * @return A new, deskewed image.
     */
    CImg<uint> deskew(const CImg<uint> &input_image);

    /**
     * @brief Enhances the contrast of the image.
     * This helps separate text from the background. Uses histogram equalization.
     * @param input_image The source image (typically grayscale).
     * @return A new, high-contrast image.
     */
    CImg<uint> contrast_enhancement(const CImg<uint> &input_image);

    /**
     * @brief Removes small connected components (speckles) from a binary image.
     * Components smaller than the specified threshold are removed.
     * @param input_image The source binary image.
     * @param threshold Components smaller than this number of pixels will be removed.
     * @param diagonal_connections Whether to consider diagonal connections as connected.
     * @return A new, despeckled image.
     */
    CImg<uint> despeckle(const CImg<uint> &input_image, uint threshold, bool diagonal_connections = true);

    /**
     * @brief Options for the `enhance` function.
     */
    struct EnhanceOptions
    {
        // --- Gaussian Denoising Options ---
        /** @brief The standard deviation for Gaussian denoising (default 1.0f). */
        float sigma = 1.0f;

        // --- Morphology Options ---
        /** @brief Whether to consider diagonal connections in despeckling (default true). */
        bool diagonal_connections = true;
        /** @brief Whether to perform erosion (default false). */
        bool do_erosion = false;
        /** @brief Whether to perform dilation (default true). */
        bool do_dilation = false;
        /** @brief Whether to perform despeckling (default true). */
        bool do_despeckle = true;
        /** @brief The structuring element size for morphological operations (default 3). */
        int kernel_size = 5;
        /** @brief The size threshold for despeckling (default 5). */
        int despeckle_threshold = 0;

        // --- Geometry Options ---
        /** @brief Whether to perform deskewing (default true). */
        bool do_deskew = false;

        // --- Sauvola Binarization Options ---
        /** @brief The size of the local window for Sauvola binarization (default: 15). */
        int sauvola_window_size = 15;
        /** @brief The sensitivity parameter 'k' for Sauvola binarization (default: 0.2f). */
        float sauvola_k = 0.2f;
        /** @brief The optional offset 'delta' for Sauvola binarization (default: 0.0f). */
        float sauvola_delta = 0.0f;

        // Optional: convenience fluent setters
        EnhanceOptions &Sigma(float v)
        {
            sigma = v;
            return *this;
        }
        EnhanceOptions &KernelSize(int v)
        {
            kernel_size = v;
            return *this;
        }
        EnhanceOptions &DespeckleThreshold(int v)
        {
            despeckle_threshold = v;
            return *this;
        }
        EnhanceOptions &DiagonalConnections(bool v)
        {
            diagonal_connections = v;
            return *this;
        }
        EnhanceOptions &Erosion(bool v = true)
        {
            do_erosion = v;
            return *this;
        }
        EnhanceOptions &Dilation(bool v = true)
        {
            do_dilation = v;
            return *this;
        }
        EnhanceOptions &Despeckle(bool v = true)
        {
            do_despeckle = v;
            return *this;
        }
        EnhanceOptions &Deskew(bool v = true)
        {
            do_deskew = v;
            return *this;
        }
        EnhanceOptions &SauvolaWindowSize(int v)
        {
            sauvola_window_size = v;
            return *this;
        }
        EnhanceOptions &SauvolaK(float v)
        {
            sauvola_k = v;
            return *this;
        }
        EnhanceOptions &SauvolaDelta(float v)
        {
            sauvola_delta = v;
            return *this;
        }
    };

    /**
     * @brief Runs the full pipeline for text enhancement.
     * This is a convenience function that chains together the most common operations.
     * @param input_image The source image.
     * @param opt The enhancement options.
     * @return An enhanced image, ready for OCR.
     */
    CImg<uint> enhance(const CImg<uint> &input_image, const EnhanceOptions &opt = {});
} // namespace ite
