#include "binarization.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include "../core/integral_image.h"


namespace ite::binarization
{

    void binarize_sauvola(CImg<uint> &input_image, const int window_size, const float k, const float delta)
    {
        if (input_image.spectrum() != 1)
        {
            throw std::runtime_error("Sauvola requires a grayscale image.");
        }

        CImg<double> img_double = input_image; // Convert CImg<uint> to CImg<double>

        CImg<double> integral_img = core::calculate_integral_image(img_double);
        CImg<double> integral_sq_img = core::calculate_integral_image(img_double.get_sqr()); // Integral of (pixel*pixel)

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
                    const double sum = core::get_area_sum(integral_img, x1, y1, z, 0, x2, y2);
                    const double sum_sq = core::get_area_sum(integral_sq_img, x1, y1, z, 0, x2, y2);

                    // Calculate local mean and std. deviation
                    const double mean = sum / N;
                    const double std_dev = std::sqrt(std::max(0.0, (sum_sq / N) - (mean * mean)));

                    // Calculate Sauvola's threshold
                    const double threshold = mean * (1.0 + k * ((std_dev / R) - 1.0)) - delta;

                    // Apply threshold
                    output_image(x, y, z) = (input_image(x, y, z) > threshold) * 255;
                }
            }
        }

        input_image = output_image;
    }

    int compute_otsu_threshold(const CImg<unsigned char> &g)
    {
        int hist[256] = {0};
        const int W = g.width(), H = g.height();
        const int N = W * H;
        if (N <= 0)
            return 128;

        const unsigned char* p = g.data();
        for (int i = 0; i < N; ++i)
            ++hist[p[i]];

        double sum_all = 0.0;
        for (int t = 0; t < 256; ++t)
            sum_all += static_cast<double>(t) * static_cast<double>(hist[t]);

        double sum_b = 0.0;
        int w_b = 0;
        int w_f = 0;

        double max_between = -1.0;
        int best_t = 128;

        for (int t = 0; t < 256; ++t)
        {
            w_b += hist[t];
            if (w_b == 0)
                continue;

            w_f = N - w_b;
            if (w_f == 0)
                break;

            sum_b += static_cast<double>(t) * static_cast<double>(hist[t]);

            const double m_b = sum_b / static_cast<double>(w_b);
            const double m_f = (sum_all - sum_b) / static_cast<double>(w_f);

            const double between = static_cast<double>(w_b) * static_cast<double>(w_f) * (m_b - m_f) * (m_b - m_f);
            if (between > max_between)
            {
                max_between = between;
                best_t = t;
            }
        }
        return best_t;
    }

    double compute_border_mean(const CImg<unsigned char> &g)
    {
        const int W = g.width(), H = g.height();
        if (W <= 0 || H <= 0)
            return 0.0;

        const int b = std::max(1, static_cast<int>(std::floor(0.05 * std::min(W, H)))); // 5% border
        const int step = 2; // subsample for speed

        uint64_t sum = 0;
        uint64_t cnt = 0;
        const unsigned char* p = g.data();

        auto add = [&](int x, int y)
        {
            sum += p[y * W + x];
            ++cnt;
        };

        // Top + bottom
        for (int y = 0; y < b; y += step)
            for (int x = 0; x < W; x += step)
                add(x, y);
        for (int y = H - b; y < H; y += step)
            for (int x = 0; x < W; x += step)
                add(x, y);

        // Left + right (excluding corners already counted)
        for (int y = b; y < H - b; y += step)
        {
            for (int x = 0; x < b; x += step)
                add(x, y);
            for (int x = W - b; x < W; x += step)
                add(x, y);
        }

        return cnt ? static_cast<double>(sum) / static_cast<double>(cnt) : 0.0;
    }

    void binarize_otsu(CImg<uint> &input_image)
    {
        if (input_image.spectrum() != 1)
        {
            throw std::runtime_error("Otsu binarization requires a grayscale image.");
        }

        // Compute Otsu's threshold and border mean
        const int threshold = compute_otsu_threshold(input_image);
        const double border_mean = compute_border_mean(input_image);

        // Determine if background is light (border mean > threshold) or dark
        const bool light_background = border_mean > static_cast<double>(threshold);

        // Binarize in-place
#pragma omp parallel for collapse(3)
        for (int z = 0; z < input_image.depth(); ++z)
        {
            for (int y = 0; y < input_image.height(); ++y)
            {
                for (int x = 0; x < input_image.width(); ++x)
                {
                    const unsigned char pixel = input_image(x, y, z);
                    // For light background: dark pixels (<=threshold) become black (0)
                    // For dark background: light pixels (>threshold) become white (255)
                    const bool is_foreground = light_background ? (pixel <= threshold) : (pixel > threshold);
                    input_image(x, y, z) = is_foreground ? 0 : 255;
                }
            }
        }
    }

    /**
     * @brief (Internal) Converts a grayscale image to a binary (black and white)
     * image, in-place. Uses simple Bataine's adaptive thresholding.
     */
    void binarize_bataineh(CImg<uint> &input_image)
    {
        /*
        Calculates adaptive binarization while using adaptive window sizes to improve
        results, as described in Bataineh et al., "An adaptive local binarization
        method for document images based on a novel thresholding method and dynamic
        windows", 2011.

        Calculation of adaptive binarization consists of two main parts:
        1. Calculation of max and min std. deviation of the adaptive window sizes to
        determine adaptive binarization thresholds.
        2. Binarization using global max and min std. deviation for each pixel's
        adaptive window.

        Calculation of adaptive window sizes is based on image characteristics and
        consists of 5 steps:
        1. Compute Confusion Threshold T_con
        2. Classify pixels based on T_con and calculation of probabilities p
        3. Determine primary window size pw_size based on probability p
        4. Set final window size W_size based on pw_size and image dimensions
        5. Use W_size for adaptive binarization
        Steps 4 and 5 are integrated in the binarization process below.
        */
        auto start = std::chrono::steady_clock::now();
        if (input_image.spectrum() != 1)
        {
            throw std::runtime_error("Adaptive Binarization requires a grayscale image.");
        }

        CImg<double> img_double = input_image; // Convert CImg<uint> to CImg<double>
        CImg<double> integral_img = core::calculate_integral_image(input_image);
        CImg<double> integral_sq_img = core::calculate_integral_image(input_image.get_sqr()); // Integral of (pixel*pixel)

        // set usefull constants
        const double mean_global = img_double.mean();
        const int img_width = img_double.width();
        const int img_height = img_double.height();
        const int img_depth = img_double.depth();

        /*----- adaptive window size steps 1-3 -----*/

        // First step: compute Confusion Threshold T_con
        const double std_dev = std::sqrt(img_double.variance());
        const double max_intensity = img_double.max();
        const double T_con = mean_global - ((mean_global * mean_global * std_dev) / ((mean_global + std_dev) * (0.5 * max_intensity + std_dev)));
        const double offset = std_dev / 2.0;

        // Second step: classify pixels based on T_con and calculation of
        // probabilities p
        long n_black = 0;
        long n_red = 0;
        long n_white = 0;
        cimg_forXYZ(img_double, x, y, z)
        {
            double pixel_value = img_double(x, y, z);
            if (pixel_value <= T_con - offset)
            {
                n_black++;
            }
            else if (pixel_value >= T_con + offset)
            {
                n_white++;
            }
            else
            {
                n_red++;
            }
        }

        double p = (n_red == 0) ? 10.0 : (double)n_black / n_red;

        // Third step: determine primary window size pw_size based on probability p
        // pw_size = [pw_size_x, pw_size_y]
        int pw_size[2];
        if (p >= 2.5 || (std_dev < 0.1 * max_intensity)) // large text size, low contact images
        {
            pw_size[0] = img_width / 6; // 6
            pw_size[1] = img_height / 4; // 4
        }
        else if (1 < p || (img_width + img_height) < 400)
        { // fine and normal images
            pw_size[0] = img_width / 30; // 30
            pw_size[1] = img_height / 20; // 20
        }
        else
        { // very fine text size, high contact images
            pw_size[0] = img_width / 40; // 40
            pw_size[1] = img_height / 30; // 30
        }

        // check if window sizes are odd numbers, if not make them odd
        if (pw_size[0] % 2 == 0)
        {
            pw_size[0]++;
        }
        if (pw_size[1] % 2 == 0)
        {
            pw_size[1]++;
        }

        // usefull constants for later
        const int pw_x_half = pw_size[0] / 2;
        const int pw_y_half = pw_size[1] / 2;

        /*----- end adaptive window size steps 1-3 -----*/

        CImg<uint> output_image(img_width, img_height, img_depth, 1);
        double min_std_dev = 255.0; // initialize to max possible value -> can only go down
        double max_std_dev = 0.0; // initialize to min possible value -> can only go up

        /*----- adaptive binarization -----*/
        // First step: Calculate local std. deviation for each window and determine
        // global min and max std. deviation
#pragma omp parallel for collapse(3) reduction(min : min_std_dev) reduction(max : max_std_dev)
        for (int z = 0; z < img_depth; ++z)
        {
            for (int y = 0; y < img_height; ++y)
            {
                for (int x = 0; x < img_width; ++x)
                {
                    // use primary window size pw_size for each pixel in step 1
                    // Define the local window (clamp to edges)
                    const int x1 = std::max(0, x - pw_x_half);
                    const int y1 = std::max(0, y - pw_y_half);
                    const int x2 = std::min(img_width - 1, x + pw_x_half);
                    const int y2 = std::min(img_height - 1, y + pw_y_half);

                    // Get sum and sum of squares from integral images
                    double sum = core::get_area_sum(integral_img, x1, y1, z, 0, x2, y2);
                    double sum_sq = core::get_area_sum(integral_sq_img, x1, y1, z, 0, x2, y2);

                    const double N = (x2 - x1 + 1) * (y2 - y1 + 1); // Number of pixels in window

                    // Calculate local mean and std. deviation
                    const double mean = sum / N;
                    const double std_dev = std::sqrt(std::max(0.0, (sum_sq / N) - (mean * mean)));

                    // set min and max global std deviation for normalization
                    if (std_dev < min_std_dev)
                    {
                        min_std_dev = std_dev;
                    }
                    if (std_dev > max_std_dev)
                    {
                        max_std_dev = std_dev;
                    }
                }
            }
        }

        // calculate range once and set a small epsilon to avoid division by zero
        const double std_dev_range = (max_std_dev - min_std_dev) > 1e-5 ? (max_std_dev - min_std_dev) : 1e-5;

        // Second pass: Binarize using local std. deviation
#pragma omp parallel for collapse(3)
        for (int z = 0; z < img_depth; ++z)
        {
            for (int y = 0; y < img_height; ++y)
            {
                for (int x = 0; x < img_width; ++x)
                {
                    // Define the local window (clamp to edges)
                    const int x1 = std::max(0, x - pw_x_half);
                    const int y1 = std::max(0, y - pw_y_half);
                    const int x2 = std::min(img_width - 1, x + pw_x_half);
                    const int y2 = std::min(img_height - 1, y + pw_y_half);

                    /*----- adaptive window size -----*/
                    // Fourth step: Set final window size W_size based on pw_size and image
                    // dimensions count number of black and red pixels in primary window
                    long n_w_black = 0;
                    long n_w_red = 0;
                    for (int i = y1; i < y2; ++i)
                    {
                        for (int j = x1; j < x2; ++j)
                        {
                            double w_pixel_value = input_image(j, i, z);
                            if (w_pixel_value <= T_con - offset)
                            {
                                n_w_black++;
                            }
                            else if (w_pixel_value < T_con + offset)
                            {
                                n_w_red++;
                            }
                        }
                    }
                    // if more red pixels than black pixels, decrease window size if not use
                    // normal pw_size
                    bool use_sub_window = (n_w_red > n_w_black);
                    int final_w_x_half = use_sub_window ? pw_x_half / 2 : pw_x_half;
                    int final_w_y_half = use_sub_window ? pw_y_half / 2 : pw_y_half;

                    // Fifth step: redefine the local window with final window size
                    const int x1_final = std::max(0, x - final_w_x_half);
                    const int y1_final = std::max(0, y - final_w_y_half);
                    const int x2_final = std::min(img_width - 1, x + final_w_x_half);
                    const int y2_final = std::min(img_height - 1, y + final_w_y_half);
                    /*----- end adaptive window size -----*/

                    // Get sum and sum of squares from integral images
                    double sum = core::get_area_sum(integral_img, x1_final, y1_final, z, 0, x2_final, y2_final);
                    double sum_sq = core::get_area_sum(integral_sq_img, x1_final, y1_final, z, 0, x2_final, y2_final);
                    const double N = (x2_final - x1_final + 1) * (y2_final - y1_final + 1); // Number of pixels in window

                    // Calculate local mean and std. deviation
                    const double mean_window_val = sum / N;
                    const double std_dev_window_val = std::sqrt(std::max(0.0, (sum_sq / N) - (mean_window_val * mean_window_val)));

                    // not part of Bataineh's method, but needed for fourther adjusting
                    // threshold
                    double k = 1.0;
                    if (std_dev_window_val < 5.0)
                    {
                        k = 1.4;
                    }
                    else if (std_dev_window_val > 30.0)
                    {
                        k = 0.8;
                    }

                    // Calculate adaptive threshold
                    double std_dev_adaptive = (std_dev_window_val - min_std_dev) / std_dev_range;

                    // define threshold based on adaptive std deviation
                    const double threshold = mean_window_val -
                        k *
                            (((mean_window_val * mean_window_val) - std_dev_window_val) /
                             ((mean_global + std_dev_window_val) * (std_dev_adaptive + std_dev_window_val)));

                    // Apply threshold
                    // new image needed because of race conditions in the parallel for loops
                    output_image(x, y, z) = (img_double(x, y, z) > threshold) * 255;
                }
            }
        }
        // needed because of race conditions in the parallel for loops
        input_image = output_image;
        auto end = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Bataineh Binarization took " << diff.count() << " ms" << std::endl;
    }

} // namespace ite::binarization
