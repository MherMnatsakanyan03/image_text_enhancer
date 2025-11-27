#include "ite.h"
#include <iostream>
#include <omp.h>

const float WEIGHT_R = 0.299f;
const float WEIGHT_G = 0.587f;
const float WEIGHT_B = 0.114f;

/**
 * @brief (Internal) Computes the Summed-Area Table (Integral Image).
 */
static CImg<double> calculate_integral_image(const CImg<double> &src)
{
    CImg<double> integral_img(src.width(), src.height(), src.depth(), src.spectrum(), 0);

    cimg_forZC(src, z, c)
    { // Process each 2D slice
        // First row
        for (int x = 0; x < src.width(); ++x)
        {
            // Add current pixel to the sum of previous pixels in the row
            integral_img(x, 0, z, c) = src(x, 0, z, c) + (x > 0 ? integral_img(x - 1, 0, z, c) : 0.0);
        }
        // Remaining rows
        for (int y = 1; y < src.height(); ++y)
        {
            double row_sum = 0.0;
            for (int x = 0; x < src.width(); ++x)
            {
                // Add current pixel to the sum of previous pixels in the row
                row_sum += src(x, y, z, c);
                integral_img(x, y, z, c) = row_sum + integral_img(x, y - 1, z, c);
            }
        }
    }

    return integral_img;
}

/**
 * @brief (Internal) Computes the sum of a region from an integral image.
 */
static double get_area_sum(const CImg<double> &integral_img,
                           int x1, int y1, int z, int c,
                           int x2, int y2)
{

    // Get sum at all four corners of the rectangle, handling boundary conditions
    const double D = integral_img(x2, y2, z, c);
    const double B = (x1 > 0) ? integral_img(x1 - 1, y2, z, c) : 0.0;
    const double C = (y1 > 0) ? integral_img(x2, y1 - 1, z, c) : 0.0;
    const double A = (x1 > 0 && y1 > 0) ? integral_img(x1 - 1, y1 - 1, z, c) : 0.0;

    // Inclusion-Exclusion Principle
    return D - B - C + A;
}

/**
 * @brief (Internal) Converts an image to grayscale, in-place.
 * Uses the standard luminance formula.
 */
static void to_grayscale_inplace(CImg<uint> &input_image)
{
    if (input_image.spectrum() == 1)
    {
        return; // Already grayscale
    }

    // Create a new image with the correct 1-channel dimensions
    CImg<uint> gray_image(input_image.width(), input_image.height(), input_image.depth(), 1);

#pragma omp parallel for collapse(2)
    for (int z = 0; z < input_image.depth(); ++z)
    {
        for (int y = 0; y < input_image.height(); ++y)
        {
            for (int x = 0; x < input_image.width(); ++x)
            {
                // Read all 3 channels
                uint r = input_image(x, y, z, 0);
                uint g = input_image(x, y, z, 1);
                uint b = input_image(x, y, z, 2);
                // Calculate and set the new pixel value
                gray_image(x, y, z, 0) = static_cast<uint>(std::round(WEIGHT_R * r + WEIGHT_G * g + WEIGHT_B * b));
            }
        }
    }

    // Assign the new 1-channel image to the input reference
    input_image = gray_image;
}

/**
 * @brief (Internal) Converts a grayscale image to a binary (black and white) image, in-place.
 * Uses Sauvola's method for adaptive thresholding.
 */
static void binarize_inplace(CImg<uint> &input_image, int window_size = 15, float k = 0.2f)
{
    if (input_image.spectrum() != 1)
    {
        throw std::runtime_error("Sauvola requires a grayscale image.");
    }

    CImg<double> img_double = input_image; // Convert CImg<uint> to CImg<double>

    CImg<double> integral_img = calculate_integral_image(img_double);
    CImg<double> integral_sq_img = calculate_integral_image(img_double.get_sqr()); // Integral of (pixel*pixel)

    CImg<uint> output_image(input_image.width(), input_image.height(), input_image.depth(), 1);
    const float R = 128.0f; // Max std. dev (for normalization)
    const int w_half = window_size / 2;

#pragma omp parallel for collapse(2)
    for (int z = 0; z < input_image.depth(); ++z)
    {
        for (int y = 0; y < input_image.height(); ++y)
        {
            for (int x = 0; x < input_image.width(); ++x)
            {
                // Define the local window (clamp to edges)
                const int x1 = std::max(0, x - w_half);
                const int y1 = std::max(0, y - w_half);
                const int x2 = std::min(input_image.width() - 1, x + w_half);
                const int y2 = std::min(input_image.height() - 1, y + w_half);

                const double N = (x2 - x1 + 1) * (y2 - y1 + 1); // Number of pixels in window

                // Get sum and sum of squares from integral images
                const double sum = get_area_sum(integral_img, x1, y1, z, 0, x2, y2);
                const double sum_sq = get_area_sum(integral_sq_img, x1, y1, z, 0, x2, y2);

                // Calculate local mean and std. deviation
                const double mean = sum / N;
                const double std_dev = std::sqrt(std::max(0.0, (sum_sq / N) - (mean * mean)));

                // Calculate Sauvola's threshold
                const double threshold = mean * (1.0 + k * ((std_dev / R) - 1.0));

                // Apply threshold
                output_image(x, y, z) = (input_image(x, y, z) > threshold) * 255;
            }
        }
    }

    input_image = output_image;
}

/**
 * @brief (Internal) Applies Gaussian denoising to an image, in-place.
 * Uses CImg's built-in blur function with neumann boundary conditions and isotropic blur.
 */
static void gaussian_denoise_inplace(CImg<uint> &input_image, float sigma)
{
    input_image.blur(sigma, 1, true);
}

/**
 * @brief (Internal) Applies dilation to a binary image, in-place.
 */
static void dilation_inplace(CImg<uint> &input_image, int kernel_size)
{
    if(input_image.spectrum() != 1)
    {
        throw std::runtime_error("Dilation requires a binary image.");
    }

    if (kernel_size <= 1) return;

    CImg<uint> source = input_image;

    int r = kernel_size / 2;
    int w = input_image.width();
    int h = input_image.height();
    int d = input_image.depth(); 

    #pragma omp parallel for collapse(2)
    for (int i_d = 0; i_d < d; ++i_d) {
        for (int i_h = 0; i_h < h; ++i_h) {
            for (int i_w = 0; i_w < w; ++i_w) {
                // If the pixel is already white, it stays white (dilation only adds white)
                if (source(i_w, i_h, i_d) == 255) {
                     continue; 
                }

                // If pixel is black, check neighbors
                bool hit = false;
                
                // Scan neighborhood
                for (int k_h = -r; k_h <= r && !hit; ++k_h) {
                    for (int k_w = -r; k_w <= r && !hit; ++k_w) {
                        int n_w = i_w + k_w;
                        int n_h = i_h + k_h;

                        // Boundary check
                        if (n_w >= 0 && n_w < w && n_h >= 0 && n_h < h) {
                            if (source(n_w, n_h, i_d) == 255) {
                                hit = true;
                            }
                        }
                    }
                }
                
                if (hit) {
                   input_image(i_w, i_h, i_d) = 255;
                }
            }
        }
    }
}

// TODO: Implement the actual image processing algorithms below
static void erosion_inplace(CImg<uint> &input_image, int kernel_size)
{
    // Placeholder for actual erosion logic
    (void)input_image;
    (void)kernel_size;
}

// TODO: Implement the actual image processing algorithms below
static void deskew_inplace(CImg<uint> &input_image)
{
    // Placeholder for actual deskewing logic
    (void)input_image;
}

// TODO: Implement the actual image processing algorithms below
static void contrast_enhancement_inplace(CImg<uint> &input_image)
{
    // Placeholder for actual contrast enhancement logic
    (void)input_image;
}

namespace ite
{
    CImg<uint> loadimage(std::string filepath)
    {
        CImg<uint> image(filepath.c_str());
        return image;
    }

    void writeimage(const CImg<uint> &image, std::string filepath)
    {
        image.save(filepath.c_str());
    }

    CImg<uint> to_grayscale(const CImg<uint> &input_image)
    {
        CImg<uint> output_image = input_image;

        to_grayscale_inplace(output_image);

        return output_image;
    }

    CImg<uint> binarize(const CImg<uint> &input_image)
    {
        CImg<uint> output_image = input_image;

        if (output_image.spectrum() != 1)
        {
            to_grayscale_inplace(output_image);
        }

        binarize_inplace(output_image);

        return output_image;
    }

    CImg<uint> gaussian_denoise(const CImg<uint> &input_image, float sigma)
    {
        CImg<uint> output_image = input_image;

        gaussian_denoise_inplace(output_image, sigma);

        return output_image;
    }

    CImg<uint> dilation(const CImg<uint> &input_image, int kernel_size)
    {
        CImg<uint> output_image = input_image;

        dilation_inplace(output_image, kernel_size);

        return output_image;
    }

    CImg<uint> erosion(const CImg<uint> &input_image, int kernel_size)
    {
        CImg<uint> output_image = input_image;

        erosion_inplace(output_image, kernel_size);

        return output_image;
    }

    CImg<uint> deskew(const CImg<uint> &input_image)
    {
        CImg<uint> output_image = input_image;

        deskew_inplace(output_image);

        return output_image;
    }

    CImg<uint> contrast_enhancement(const CImg<uint> &input_image)
    {
        CImg<uint> output_image = input_image;

        contrast_enhancement_inplace(output_image);

        return output_image;
    }

    CImg<uint> enhance(const CImg<uint> &input_image, float sigma, int kernel_size)
    {
        CImg<uint> l_image = input_image;

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