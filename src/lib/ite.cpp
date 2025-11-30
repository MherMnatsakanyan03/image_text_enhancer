#include "ite.h"
#include <iostream>
#include <omp.h>
#include <vector>

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
    if (input_image.spectrum() != 1)
    {
        throw std::runtime_error("Dilation requires a single-channel image.");
    }

    if (kernel_size <= 1)
    {
        return;
    }

    CImg<uint> source = input_image;

    int r = kernel_size / 2;
    int w = input_image.width();
    int h = input_image.height();
    int d = input_image.depth();

#pragma omp parallel for collapse(2)
    for (int i_d = 0; i_d < d; ++i_d)
    {
        for (int i_h = 0; i_h < h; ++i_h)
        {
            for (int i_w = 0; i_w < w; ++i_w)
            {
                // If the pixel is already white, it stays white (dilation only adds white)
                if (source(i_w, i_h, i_d) == 255)
                {
                    continue;
                }

                // If pixel is black, check neighbors
                bool hit = false;

                // Scan neighborhood
                for (int k_h = -r; k_h <= r && !hit; ++k_h)
                {
                    for (int k_w = -r; k_w <= r && !hit; ++k_w)
                    {
                        int n_w = i_w + k_w;
                        int n_h = i_h + k_h;

                        // Boundary check
                        if (n_w >= 0 && n_w < w && n_h >= 0 && n_h < h)
                        {
                            if (source(n_w, n_h, i_d) == 255)
                            {
                                hit = true;
                            }
                        }
                    }
                }

                if (hit)
                {
                    input_image(i_w, i_h, i_d) = 255;
                }
            }
        }
    }
}

/**
 * @brief (Internal) Applies erosion to a binary image, replacing the original image.
 */
static void erosion_inplace(CImg<uint> &input_image, int kernel_size)
{
    if (input_image.spectrum() != 1)
    {
        throw std::runtime_error("Erosion requires a grayscale image.");
    }

    if (kernel_size <= 1)
        return;

    CImg<uint> source = input_image;

    int r = kernel_size / 2;
    int w = input_image.width();
    int h = input_image.height();
    int d = input_image.depth();

#pragma omp parallel for collapse(2)
    for (int i_d = 0; i_d < d; ++i_d)
    {
        for (int i_h = 0; i_h < h; ++i_h)
        {
            for (int i_w = 0; i_w < w; ++i_w)
            {
                // If the pixel is already black, it stays black (erosion only removes white)
                if (source(i_w, i_h, i_d) == 0)
                {
                    continue;
                }

                // If pixel is white, check neighbors
                bool hit = false;

                // Scan neighborhood
                for (int k_h = -r; k_h <= r && !hit; ++k_h)
                {
                    for (int k_w = -r; k_w <= r && !hit; ++k_w)
                    {
                        int n_w = i_w + k_w;
                        int n_h = i_h + k_h;

                        // Boundary check
                        if (n_w >= 0 && n_w < w && n_h >= 0 && n_h < h)
                        {
                            if (source(n_w, n_h, i_d) == 0)
                            {
                                hit = true;
                            }
                        }
                    }
                }

                if (hit)
                {
                    input_image(i_w, i_h, i_d) = 0;
                }
            }
        }
    }
}

// TODO: Implement the actual image processing algorithms below
static void deskew_inplace(CImg<uint> &input_image)
{
    // Placeholder for actual deskewing logic
    (void)input_image;
}

/**
 * @brief (Internal) Robust Linear Contrast Stretching.
 * Clips the bottom 1% and top 1% of intensities to ignore outliers,
 * then stretches the remaining range to 0-255.
 */
static void contrast_enhancement_inplace(CImg<uint> &input_image)
{
    if (input_image.is_empty())
    {
        return;
    }

    // Compute Histogram (to find percentiles)
    // CImg histogram is fast and safe.
    const CImg<uint> hist = input_image.get_histogram(256, 0, 255);
    const uint total_pixels = input_image.size();

    // Find lower (1%) and upper (99%) cutoffs
    const uint cutoff = total_pixels / 100; // 1% threshold

    uint min_val = 0;
    uint count = 0;
    // find brightest pixel above cutoff
    for (int i = 0; i < 256; ++i)
    {
        count += hist[i];
        if (count > cutoff)
        {
            min_val = i;
            break;
        }
    }

    // find darkest pixel below cutoff
    uint max_val = 255;
    count = 0;
    for (int i = 255; i >= 0; --i)
    {
        count += hist[i];
        if (count > cutoff)
        {
            max_val = i;
            break;
        }
    }

    // Safety check: if image is solid color, min might equal max
    if (max_val <= min_val)
    {
        return;
    }

    // Apply the stretch
    // Formula: 255 * (val - min) / (max - min)
    const float scale = 255.0f / (max_val - min_val);

#pragma omp parallel for
    for (uint i = 0; i < total_pixels; ++i)
    {
        uint val = input_image[i];

        if (val <= min_val)
        {
            input_image[i] = 0;
        }
        else if (val >= max_val)
        {
            input_image[i] = 255;
        }
        else
        {
            input_image[i] = (val - min_val) * scale;
        }
    }
}

/**
 * @brief (Internal) Removes small connected components (speckles) from the image.
 * Assumes the input is binary (0 and 255).
 * @param input_image The image to process.
 * @param size_threshold Components smaller than this number of pixels will be removed.
 */
static void despeckle_inplace(CImg<uint> &input_image, uint threshold, bool diagonal_connections = true)
{
    if (threshold <= 0)
        return;

// Invert image so Text/Noise becomes White (255) and Background becomes Black (0).
// CImg's label() function tracks non-zero regions.
#pragma omp parallel for
    for (uint i = 0; i < input_image.size(); ++i)
    {
        input_image[i] = (input_image[i] == 0) ? 255 : 0;
    }

    // Label connected components.
    // This assigns a unique integer ID (1, 2, 3...) to every distinct blob.
    // 0 is background.
    // true = 8-connectivity (diagonal pixels connect), false = 4-connectivity
    CImg<uint> labels = input_image.get_label(diagonal_connections);

    // Count the size of each component.
    uint max_label = labels.max();
    if (max_label == 0)
    {
        // Image was completely empty (or full), revert inversion and return
        input_image.fill(255);
        return;
    }

    std::vector<uint> sizes(max_label + 1, 0);

    // Iterate over the label image to count pixels per label
    // (This part is hard to parallelize efficiently due to random access writes)
    cimg_for(labels, ptr, uint)
    {
        sizes[*ptr]++;
    }

// Filter: If a label is too small, turn it off (Black -> 0) in our inverted map
#pragma omp parallel for collapse(2)
    for (int z = 0; z < input_image.depth(); ++z)
    {
        for (int y = 0; y < input_image.height(); ++y)
        {
            for (int x = 0; x < input_image.width(); ++x)
            {
                uint label_id = labels(x, y, z);
                // If it's a valid object (id > 0) AND it's smaller than threshold
                if (label_id > 0 && sizes[label_id] < threshold)
                {
                    input_image(x, y, z) = 0; // Erase it (in inverted space)
                }
            }
        }
    }

// Invert back to original (Black Text, White Background)
#pragma omp parallel for
    for (uint i = 0; i < input_image.size(); ++i)
    {
        input_image[i] = (input_image[i] == 255) ? 0 : 255;
    }
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

    CImg<uint> despeckle(const CImg<uint> &input_image, uint threshold, bool diagonal_connections)
    {
        CImg<uint> output_image = input_image;

        despeckle_inplace(output_image, threshold, diagonal_connections);

        return output_image;
    }

    CImg<uint> enhance(
        const CImg<uint> &input_image,
        float sigma,
        int kernel_size,
        int despeckle_threshold,
        bool diagonal_connections,
        bool do_erosion,
        bool do_dilation,
        bool do_despeckle,
        bool do_deskew)
    {
        CImg<uint> l_image = input_image;

        to_grayscale_inplace(l_image);
        if (do_deskew)
        {
            deskew_inplace(l_image);
        }
        gaussian_denoise_inplace(l_image, sigma);
        contrast_enhancement_inplace(l_image);
        binarize_inplace(l_image);
        if (do_dilation)
        {
            dilation_inplace(l_image, kernel_size);
        }
        if (do_erosion)
        {
            erosion_inplace(l_image, kernel_size);
        }
        if (do_despeckle)
        {
            despeckle_inplace(l_image, despeckle_threshold, diagonal_connections);
        }

        return l_image;
    }
}