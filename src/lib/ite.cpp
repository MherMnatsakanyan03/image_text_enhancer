#include "ite.h"
#include <iostream>
#include <omp.h>

const float WEIGHT_R = 0.299f;
const float WEIGHT_G = 0.587f;
const float WEIGHT_B = 0.114f;

/**
 * @brief (Internal) Converts an image to grayscale, in-place.
 * Uses the standard luminance formula.
 */
static void to_grayscale_inplace(CImg<unsigned char> &input_image)
{
    if (input_image.spectrum() == 1)
    {
        return; // Already grayscale
    }

    // Create a new image with the correct 1-channel dimensions
    CImg<unsigned char> gray_image(input_image.width(), input_image.height(), input_image.depth(), 1);

#pragma omp parallel for collapse(2)
    for (int z = 0; z < input_image.depth(); ++z)
    {
        for (int y = 0; y < input_image.height(); ++y)
        {
            for (int x = 0; x < input_image.width(); ++x)
            {
                // Read all 3 channels
                unsigned char r = input_image(x, y, z, 0);
                unsigned char g = input_image(x, y, z, 1);
                unsigned char b = input_image(x, y, z, 2);
                // Calculate and set the new pixel value
                gray_image(x, y, z, 0) = static_cast<unsigned char>(std::round(WEIGHT_R * r + WEIGHT_G * g + WEIGHT_B * b));
            }
        }
    }

    // Assign the new 1-channel image to the input reference
    input_image = gray_image;
}

// TODO: Implement the actual image processing algorithms below
static void binarize_inplace(CImg<unsigned char> &input_image)
{
    // Placeholder for actual binarization logic
    (void)input_image;
}

// TODO: Implement the actual image processing algorithms below
static void gaussian_denoise_inplace(CImg<unsigned char> &input_image, float sigma)
{
    // Placeholder for actual Gaussian denoising logic
    (void)input_image;
    (void)sigma;
}

// TODO: Implement the actual image processing algorithms below
static void dilation_inplace(CImg<unsigned char> &input_image, int kernel_size)
{
    // Placeholder for actual dilation logic
    (void)input_image;
    (void)kernel_size;
}

// TODO: Implement the actual image processing algorithms below
static void erosion_inplace(CImg<unsigned char> &input_image, int kernel_size)
{
    // Placeholder for actual erosion logic
    (void)input_image;
    (void)kernel_size;
}

// TODO: Implement the actual image processing algorithms below
static void deskew_inplace(CImg<unsigned char> &input_image)
{
    // Placeholder for actual deskewing logic
    (void)input_image;
}

// TODO: Implement the actual image processing algorithms below
static void contrast_enhancement_inplace(CImg<unsigned char> &input_image)
{
    // Placeholder for actual contrast enhancement logic
    (void)input_image;
}

namespace ite
{
    CImg<unsigned char> loadimage(std::string filepath)
    {
        CImg<unsigned char> image(filepath.c_str());
        return image;
    }

    void writeimage(const CImg<unsigned char> &image, std::string filepath)
    {
        image.save(filepath.c_str());
    }

    CImg<unsigned char> to_grayscale(const CImg<unsigned char> &input_image)
    {
        CImg<unsigned char> output_image = input_image;

        to_grayscale_inplace(output_image);

        return output_image;
    }

    CImg<unsigned char> binarize(const CImg<unsigned char> &input_image)
    {
        CImg<unsigned char> output_image = input_image;

        binarize_inplace(output_image);

        return output_image;
    }

    CImg<unsigned char> gaussian_denoise(const CImg<unsigned char> &input_image, float sigma)
    {
        CImg<unsigned char> output_image = input_image;

        gaussian_denoise_inplace(output_image, sigma);

        return output_image;
    }

    CImg<unsigned char> dilation(const CImg<unsigned char> &input_image, int kernel_size)
    {
        CImg<unsigned char> output_image = input_image;

        dilation_inplace(output_image, kernel_size);

        return output_image;
    }

    CImg<unsigned char> erosion(const CImg<unsigned char> &input_image, int kernel_size)
    {
        CImg<unsigned char> output_image = input_image;

        erosion_inplace(output_image, kernel_size);

        return output_image;
    }

    CImg<unsigned char> deskew(const CImg<unsigned char> &input_image)
    {
        CImg<unsigned char> output_image = input_image;

        deskew_inplace(output_image);

        return output_image;
    }

    CImg<unsigned char> contrast_enhancement(const CImg<unsigned char> &input_image)
    {
        CImg<unsigned char> output_image = input_image;

        contrast_enhancement_inplace(output_image);

        return output_image;
    }

    CImg<unsigned char> enhance(const CImg<unsigned char> &input_image, float sigma, int kernel_size)
    {
        CImg<unsigned char> l_image = input_image;

        to_grayscale_inplace(l_image);
        deskew_inplace(l_image);
        gaussian_denoise_inplace(l_image, sigma);
        contrast_enhancement_inplace(l_image);
        binarize_inplace(l_image);
        dilation_inplace(l_image, kernel_size);
        erosion_inplace(l_image, kernel_size);

        return l_image;
    }
}