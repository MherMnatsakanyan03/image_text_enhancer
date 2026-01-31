#include "binarization.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "../core/integral_image.h"


namespace ite::binarization
{
    void binarize_sauvola(CImg<uint> &input_image, const int window_size, const float k, const float delta)
    {
        if (input_image.spectrum() != 1)
        {
            throw std::runtime_error("Sauvola requires a grayscale image.");
        }

        const int w = input_image.width();
        const int h = input_image.height();
        const int d = input_image.depth();

        const float R = 128.0f;
        const int w_half = window_size / 2;
        const int stride = w + 1;

        // Buffers for Integral Images (Reused across Z slices)
        // This avoids allocating 3 full CImg objects per slice.
        std::vector<double> integral_img;
        std::vector<double> integral_img_sqr;

        CImg<uint> output_image(w, h, d, 1);

        // Process each depth slice independently
        for (int z = 0; z < d; ++z)
        {
            // 1. Build Integral Images (Fused & Parallel)
            core::compute_fused_integrals(input_image, z, integral_img, integral_img_sqr);

            // 2. Apply Sauvola (Parallel)
#pragma omp parallel for collapse(2)
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    // Window coordinates (clamped to image 0..w-1)
                    // Padded Integral Image logic handles -1 implicitly via 0-row/col
                    const int x1 = std::max(0, x - w_half);
                    const int y1 = std::max(0, y - w_half);
                    const int x2 = std::min(w - 1, x + w_half);
                    const int y2 = std::min(h - 1, y + w_half);

                    // Integral Image Coordinates (Padded: +1)
                    // D(x2, y2) - B(x1-1, y2) - C(x2, y1-1) + A(x1-1, y1-1)
                    // Since x1/y1 are inclusive, "left/top" is x1, so we access index x1 (which corresponds to x1-1 in 0-based unpadded logic)
                    // Wait:
                    // Padded Index i maps to Image Pixel i-1.
                    // Sum(0..i) is stored at index i+1.
                    // Sum(x1..x2) = Sum(0..x2) - Sum(0..x1-1).
                    // In padded array: Data[x2+1] - Data[x1].

                    const int idx_D = (y2 + 1) * stride + (x2 + 1);
                    const int idx_B = (y2 + 1) * stride + x1;
                    const int idx_C = y1 * stride + (x2 + 1);
                    const int idx_A = y1 * stride + x1;

                    const double sum = integral_img[idx_D] - integral_img[idx_B] - integral_img[idx_C] + integral_img[idx_A];
                    const double sum_sq = integral_img_sqr[idx_D] - integral_img_sqr[idx_B] - integral_img_sqr[idx_C] + integral_img_sqr[idx_A];

                    const double N = (double)((x2 - x1 + 1) * (y2 - y1 + 1));

                    const double mean = sum / N;
                    const double var = (sum_sq / N) - (mean * mean);
                    const double std_dev = std::sqrt(std::max(0.0, var));

                    const double threshold = mean * (1.0 + k * ((std_dev / R) - 1.0)) - delta;

                    output_image(x, y, z) = (input_image(x, y, z) > threshold) ? 255 : 0;
                }
            }
        }

        input_image.swap(output_image);
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
        if (input_image.spectrum() != 1)
            throw std::runtime_error("Adaptive Binarization requires a grayscale image.");

        const int w = input_image.width();
        const int h = input_image.height();

        // 1. Build Standard Integral Images using
        std::vector<double> integral_img;
        std::vector<double> integral_img_sqr;
        core::compute_fused_integrals(input_image, 0, integral_img, integral_img_sqr);

        // Calculate global stats (using integral image for the whole area 0,0 to w-1,h-1)
        double total_sum = core::get_sum_padded(integral_img, w, 0, 0, w - 1, h - 1);
        double total_sq = core::get_sum_padded(integral_img_sqr, w, 0, 0, w - 1, h - 1);
        double N_total = (double)(w * h);

        const double mean_global = total_sum / N_total;
        const double var_global = (total_sq / N_total) - (mean_global * mean_global);
        const double std_dev_global = std::sqrt(std::max(0.0, var_global));

        // Find max intensity
        uint max_val_int = 0;
#pragma omp parallel for reduction(max : max_val_int)
        for (size_t i = 0; i < input_image.size(); ++i)
        {
            if (input_image[i] > max_val_int)
                max_val_int = input_image[i];
        }
        const double max_intensity = (double)max_val_int;

        // 2. Compute Confusion Threshold T_con
        const double T_con =
            mean_global - ((mean_global * mean_global * std_dev_global) / ((mean_global + std_dev_global) * (0.5 * max_intensity + std_dev_global)));
        const double offset = std_dev_global / 2.0;

        // 3. Classify Pixels & Build Count Integral Images
        // Create masks: 1.0 where condition is true, 0.0 otherwise
        CImg<double> mask_black(w, h, 1, 1, 0);
        CImg<double> mask_red(w, h, 1, 1, 0);

        long n_black_total = 0;
        long n_red_total = 0;

#pragma omp parallel for reduction(+ : n_black_total, n_red_total)
        for (int i = 0; i < w * h; ++i)
        {
            double val = (double)input_image[i];
            if (val <= T_con - offset)
            {
                mask_black[i] = 1.0;
                n_black_total++;
            }
            else if (val < T_con + offset)
            {
                mask_red[i] = 1.0;
                n_red_total++;
            }
        }

        // Create Integral Images for the Counts
        CImg<double> integral_img_black = core::calculate_integral_image(mask_black);
        CImg<double> integral_img_red = core::calculate_integral_image(mask_red);

        // 4. Determine Primary Window Size (pw_size)
        double p = (n_red_total == 0) ? 10.0 : (double)n_black_total / n_red_total;

        int pw_size_x, pw_size_y;
        if (p >= 2.5 || (std_dev_global < 0.1 * max_intensity))
        {
            pw_size_x = w / 6;
            pw_size_y = h / 4;
        }
        else if (1 < p || (w + h) < 400)
        {
            pw_size_x = w / 30;
            pw_size_y = h / 20;
        }
        else
        {
            pw_size_x = w / 40;
            pw_size_y = h / 30;
        }

        if (pw_size_x % 2 == 0)
            pw_size_x++;
        if (pw_size_y % 2 == 0)
            pw_size_y++;

        const int pw_x_half = pw_size_x / 2;
        const int pw_y_half = pw_size_y / 2;

        // 5. Calculate global Min/Max local Std Dev
        // We use the primary window size
        double min_std_dev = 255.0;
        double max_std_dev = 0.0;

#pragma omp parallel for collapse(2) reduction(min : min_std_dev) reduction(max : max_std_dev)
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const int x1 = std::max(0, x - pw_x_half);
                const int y1 = std::max(0, y - pw_y_half);
                const int x2 = std::min(w - 1, x + pw_x_half);
                const int y2 = std::min(h - 1, y + pw_y_half);

                double N = (double)((x2 - x1 + 1) * (y2 - y1 + 1));
                double s = core::get_sum_padded(integral_img, w, x1, y1, x2, y2);
                double sq = core::get_sum_padded(integral_img_sqr, w, x1, y1, x2, y2);

                double m = s / N;
                double v = (sq / N) - (m * m);
                double sd = std::sqrt(std::max(0.0, v));

                if (sd < min_std_dev)
                    min_std_dev = sd;
                if (sd > max_std_dev)
                    max_std_dev = sd;
            }
        }

        const double std_dev_range = (max_std_dev - min_std_dev) > 1e-5 ? (max_std_dev - min_std_dev) : 1e-5;
        CImg<uint> output_image(w, h, 1, 1);

// 6. Final Binarization Pass
#pragma omp parallel for collapse(2)
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                // Primary Window Coords
                const int x1 = std::max(0, x - pw_x_half);
                const int y1 = std::max(0, y - pw_y_half);
                const int x2 = std::min(w - 1, x + pw_x_half);
                const int y2 = std::min(h - 1, y + pw_y_half);

                // OPTIMIZATION: Get counts from Integral Images (O(1))
                double n_w_black = core::get_area_sum(integral_img_black, x1, y1, 0, 0, x2, y2);
                double n_w_red = core::get_area_sum(integral_img_red, x1, y1, 0, 0, x2, y2);

                bool use_sub_window = (n_w_red > n_w_black);

                int curr_x_half = use_sub_window ? pw_x_half / 2 : pw_x_half;
                int curr_y_half = use_sub_window ? pw_y_half / 2 : pw_y_half;

                // Final Window Coords
                const int x1_f = std::max(0, x - curr_x_half);
                const int y1_f = std::max(0, y - curr_y_half);
                const int x2_f = std::min(w - 1, x + curr_x_half);
                const int y2_f = std::min(h - 1, y + curr_y_half);

                double N = (double)((x2_f - x1_f + 1) * (y2_f - y1_f + 1));
                double s = core::get_sum_padded(integral_img, w, x1_f, y1_f, x2_f, y2_f);
                double sq = core::get_sum_padded(integral_img_sqr, w, x1_f, y1_f, x2_f, y2_f);

                double mean_w = s / N;
                double var_w = (sq / N) - (mean_w * mean_w);
                double std_dev_w = std::sqrt(std::max(0.0, var_w));

                double k = 1.0;
                if (std_dev_w < 5.0)
                    k = 1.4;
                else if (std_dev_w > 30.0)
                    k = 0.8;

                double std_dev_adaptive = (std_dev_w - min_std_dev) / std_dev_range;

                double term = ((mean_w * mean_w) - std_dev_w) / ((mean_global + std_dev_w) * (std_dev_adaptive + std_dev_w));

                double threshold = mean_w - k * term;

                output_image(x, y) = (input_image(x, y) > threshold) ? 255 : 0;
            }
        }

        input_image.swap(output_image);
    }

} // namespace ite::binarization
