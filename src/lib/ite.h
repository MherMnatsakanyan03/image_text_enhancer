#pragma once
#include "CImg.h"
#include <string>

// Use the CImg library namespace.
using namespace cimg_library;

/**
 * @namespace ite
 * @brief Contains functions for Image Text Enhancement (ITE) to improve OCR readability.
 */
namespace ite {

    /**
     * @brief Loads an image from a specified file path.
     * @param filepath The relative or absolute path to the image file.
     * @return A CImg<unsigned char> object containing the image data.
     * @throws CImgIOException if the file cannot be opened or recognized.
     */
    CImg<unsigned char> loadimage(std::string filepath);

    /**
     * @brief Saves an image to a specified file path.
     * @param image The CImg<unsigned char> object containing the image data to save.
     * @param filepath The relative or absolute path where the image will be saved.
     * @throws CImgIOException if the file cannot be written.
     */
    void writeimage(const CImg<unsigned char>& image, std::string filepath);

    /**
     * @brief Converts an image to grayscale.
     * If the image is already 1-channel, a copy is returned.
     * @param input_image The source image (can be 1, 3, or 4 channels).
     * @return A new 1-channel (grayscale) image.
     */
    CImg<unsigned char> to_grayscale(const CImg<unsigned char>& input_image);

    /**
     * @brief Converts a grayscale image to a binary (black and white) image.
     * This is critical for OCR. Uses an adaptive thresholding method.
     * @param input_image The source grayscale image.
     * @return A new 1-channel binary image (values 0 or 255).
     */
    CImg<unsigned char> binarize(const CImg<unsigned char>& input_image);

    /**
     * @brief Applies a Gaussian blur to denoise the image.
     * Best used on a grayscale image before binarization.
     * @param input_image The source image.
     * @param sigma The standard deviation of the Gaussian kernel. Larger values mean more blur.
     * @return A new, blurred image.
     */
    CImg<unsigned char> gaussian_denoise(const CImg<unsigned char>& input_image, float sigma = 1.0f);

    /**
     * @brief Performs morphological dilation on the image.
     * This operation thickens bright regions. On a binary image, it connects broken character parts.
     * @param input_image The source image (typically binary).
     * @param kernel_size The size of the structuring element (e.g., 3 for a 3x3 kernel).
     * @return A new, dilated image.
     */
    CImg<unsigned char> dilation(const CImg<unsigned char>& input_image, int kernel_size = 3);
    
    /**
     * @brief Performs morphological erosion on the image.
     * This operation thickens dark regions. On a binary image, it removes small noise specks.
     * @param input_image The source image (typically binary).
     * @param kernel_size The size of the structuring element (e.g., 3 for a 3x3 kernel).
     * @return A new, eroded image.
     */
    CImg<unsigned char> erosion(const CImg<unsigned char>& input_image, int kernel_size = 3);

    /**
     * @brief Detects and corrects for skew (rotation) in the image.
     * Finds the dominant text angle and rotates the image to be level.
     * @param input_image The source image (typically grayscale).
     * @return A new, deskewed image.
     */
    CImg<unsigned char> deskew(const CImg<unsigned char>& input_image);

    /**
     * @brief Enhances the contrast of the image.
     * This helps separate text from the background. Uses histogram equalization.
     * @param input_image The source image (typically grayscale).
     * @return A new, high-contrast image.
     */
    CImg<unsigned char> contrast_enhancement(const CImg<unsigned char>& input_image);

    /**
     * @brief Runs the full pipeline for text enhancement.
     * Applies a logical sequence of operations (e.g., grayscale, denoise, binarize)
     * to produce an image optimized for OCR.
     * @param input_image The original, unprocessed image.
     * @param sigma The standard deviation for Gaussian denoising (default 1.0f).
     * @param kernel_size The structuring element size for morphological operations (default 3).
     * @return A new, enhanced image ready for OCR.
     */
    CImg<unsigned char> enhance(const CImg<unsigned char>& input_image, float sigma = 1.0f, int kernel_size = 3);

}