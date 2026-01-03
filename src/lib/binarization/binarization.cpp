#include "binarization.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include "../core/integral_image.h"


namespace ite::binarization
{

    void binarize_sauvola(CImg<uint> &input_image, const int window_size, const float k, const float delta)
    {
        if (input_image.spectrum() != 1)
        {
            throw std::runtime_error("Sauvola binarization requires a grayscale image.");
        }

        CImg<double> img_double = input_image;

        CImg<double> integral_img = core::calculate_integral_image(img_double);
        CImg<double> integral_sq_img = core::calculate_integral_image(img_double.get_sqr());

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

                    const double N = (x2 - x1 + 1) * (y2 - y1 + 1);

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

} // namespace ite::binarization
