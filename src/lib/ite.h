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
     * @brief Applies a simple Gaussian blur to blur the image.
     * Best used on a grayscale image before binarization.
     * @param input_image The source image.
     * @param sigma The standard deviation of the Gaussian kernel. Larger values mean more blur.
     * @param boundary_conditions The type of boundary conditions to use. See `CImg::blur()` in `CImg.h` for more info (default: 1 = Neumann).
     * @return A new, blurred image.
     */
    CImg<uint> simple_gaussian_blur(const CImg<uint> &input_image, float sigma = 1.0f, int boundary_conditions = 1);

    /**
     * @brief Applies an adaptive Gaussian blur to the image.
     * This method blurs less around edges and more in flat regions.
     * @param input_image The source image.
     * @param sigma_low The standard deviation for edge regions.
     * @param sigma_high The standard deviation for flat regions.
     * @param edge_thresh The variance threshold to distinguish edges.
     * @param truncate Factor to determine kernel size from sigma (default: 3).
     * @param block_h Height of the blocks for parallel processing (default: 64).
     * @return A new, adaptively blurred image.
     */
    CImg<uint> adaptive_gaussian_blur(const CImg<uint> &input_image, float sigma_low, float sigma_high, float edge_thresh, int truncate = 3, int block_h = 64);

    /**
     * @brief Applies an adaptive median filter to the image.
     * Excellent for removing impulse noise (salt-and-pepper) while preserving edges.
     * Starts with a 3x3 window and expands when detecting impulse noise.
     * @param input_image The source image.
     * @param max_window_size Maximum window size (must be odd, >= 3, typical: 5, 7, or 9).
     * @param block_h Height of the blocks for parallel processing (default: 64).
     * @return A new, filtered image.
     */
    CImg<uint> adaptive_median_filter(const CImg<uint> &input_image, int max_window_size = 7, int block_h = 64);

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
     * @param boundary_conditions The type of boundary conditions to use. See `CImg::blur()` in `CImg.h` for more info (default: 1 = Neumann).
     * @return A new, high-contrast image.
     */
    CImg<uint> contrast_enhancement(const CImg<uint> &input_image, int boundary_conditions = 1);

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
        /** brief Boundary conditions for filters (default 1, Neumann). See CImg::blur() for details. */
        int boundary_conditions = 1;

        // --- Gaussian Denoising Options ---
        /** @brief Whether to perform Gaussian denoising (default false). */
        bool do_gaussian_blur = false;
        /** @brief Whether to perform median denoising (default false). */
        bool do_median_blur = false;
        /** @brief Whether to perform adaptive median filtering (default false). */
        bool do_adaptive_median = false;
        /** @brief Whether to perform adaptive Gaussian denoising (default false). */
        bool do_adaptive_gaussian_blur = false;
        /** @brief The standard deviation for Gaussian denoising (default 1.0f). */
        float sigma = 1.0f;
        /** @brief The low standard deviation for adaptive Gaussian denoising (default 0.5f). */
        float adaptive_sigma_low = 0.5f;
        /** @brief The high standard deviation for adaptive Gaussian denoising (default 2.0f). */
        float adaptive_sigma_high = 2.0f;
        /** @brief The edge threshold for adaptive Gaussian denoising (default 30.0f). */
        float adaptive_edge_thresh = 30.0f;
        /** @brief The kernel size for median denoising (default 3). */
        int median_kernel_size = 3;
        /** @brief The threshold for median denoising (default 0). */
        unsigned int median_threshold = 0;
        /** @brief Maximum window size for adaptive median filter (must be odd, default 7). */
        int adaptive_median_max_window = 7;

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

        EnhanceOptions &BoundaryConditions(int v)
        {
            boundary_conditions = v;
            return *this;
        }

        EnhanceOptions &GaussianBlur(bool v = true)
        {
            do_gaussian_blur = v;
            return *this;
        }
        EnhanceOptions &MedianBlur(bool v = true)
        {
            do_median_blur = v;
            return *this;
        }
        EnhanceOptions &AdaptiveMedian(bool v = true)
        {
            do_adaptive_median = v;
            return *this;
        }
        EnhanceOptions &AdaptiveGaussianBlur(bool v = true)
        {
            do_adaptive_gaussian_blur = v;
            return *this;
        }
        EnhanceOptions &Sigma(float v)
        {
            sigma = v;
            return *this;
        }
        EnhanceOptions &AdaptiveSigmaLow(float v)
        {
            adaptive_sigma_low = v;
            return *this;
        }
        EnhanceOptions &AdaptiveSigmaHigh(float v)
        {
            adaptive_sigma_high = v;
            return *this;
        }
        EnhanceOptions &AdaptiveEdgeThresh(float v)
        {
            adaptive_edge_thresh = v;
            return *this;
        }
        EnhanceOptions &MedianKernelSize(int v)
        {
            median_kernel_size = v;
            return *this;
        }
        EnhanceOptions &MedianThreshold(unsigned int v)
        {
            median_threshold = v;
            return *this;
        }
        EnhanceOptions &AdaptiveMedianMaxWindow(int v)
        {
            adaptive_median_max_window = v;
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
