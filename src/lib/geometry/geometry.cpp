#include "geometry.h"
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>
#include "../binarization/binarization.h"
#include "../core/utils.h"
#include "color/grayscale.h"

using namespace ite::utils;

namespace ite::geometry
{
    struct Point
    {
        int x, y;
    };

    // Optimized Score: Projects points onto a rotated vertical axis (Radon Transform)
    static double score_angle_radon(const std::vector<Point> &points, int w, int h, double angle_deg)
    {
        if (points.empty())
            return 0.0;

        // Convert to radians
        const double rad = angle_deg * M_PI / 180.0;
        const double sin_a = std::sin(rad);
        const double cos_a = std::cos(rad);

        // Determine histogram size (diagonal covers all possible projections)
        const int max_dim = w + h;
        const int center_offset = max_dim; // Offset to handle negative projections
        std::vector<int> hist(max_dim * 2, 0);

        // Project every pixel: y' = x * -sin(a) + y * cos(a)
        for (const auto &p : points)
        {
            int y_rot = static_cast<int>(-p.x * sin_a + p.y * cos_a);
            int idx = y_rot + center_offset;

            if (idx >= 0 && idx < (int)hist.size())
            {
                hist[idx]++;
            }
        }

        // Calculate Sum of Squares (Energy)
        double sum_sq = 0.0;
        for (int count : hist)
        {
            if (count > 0)
            {
                sum_sq += static_cast<double>(count) * static_cast<double>(count);
            }
        }

        return sum_sq;
    }

    static std::pair<double, double> search_best_angle_radon(const std::vector<Point> &points, int w, int h, double start_deg, double end_deg, double step_deg)
    {
        if (step_deg <= 0.0)
            return {0.0, -1.0};
        if (end_deg < start_deg)
            std::swap(start_deg, end_deg);

        const int N_steps = static_cast<int>(std::floor((end_deg - start_deg) / step_deg)) + 1;

        double best_angle = 0.0;
        double best_score = -1.0;

#pragma omp parallel
        {
            double local_best_a = 0.0;
            double local_best_s = -1.0;

#pragma omp for schedule(static) nowait
            for (int i = 0; i < N_steps; ++i)
            {
                const double a = start_deg + static_cast<double>(i) * step_deg;
                const double s = score_angle_radon(points, w, h, a);
                if (s > local_best_s)
                {
                    local_best_s = s;
                    local_best_a = a;
                }
            }

#pragma omp critical
            {
                if (local_best_s > best_score)
                {
                    best_score = local_best_s;
                    best_angle = local_best_a;
                }
            }
        }

        return {best_angle, best_score};
    }

    double detect_skew_angle(const CImg<uint> &input_image, const int window_size, const float k, const float delta)
    {
        const int inW = input_image.width();
        const int inH = input_image.height();
        if (inW <= 1 || inH <= 1)
            return 0.0;

        // Downscale for speed (Target 600px long side)
        // This makes Sauvola fast because the image is small.
        constexpr double target_long = 600.0;
        const int long_side = std::max(inW, inH);
        double scale = target_long / static_cast<double>(long_side);
        if (scale > 1.0)
            scale = 1.0;

        int new_w = std::max(1, static_cast<int>(std::lround(inW * scale)));
        int new_h = std::max(1, static_cast<int>(std::lround(inH * scale)));

        CImg<uint> small = input_image.get_resize(new_w, new_h, 1, input_image.spectrum());
        ite::color::to_grayscale_rec601(small);

        // We use Sauvola here because it handles shading better than Otsu,
        // ensuring we actually get text lines even in shadowed corners.
        // Parameters: window=15, k=0.2, delta=10 (Standard robust defaults)
        binarization::binarize_sauvola(small, window_size, k, delta);

        // Extract Foreground Points
        // Sauvola outputs 0 (black/text) and 255 (white/background).
        // We just need to collect the 0s (or 255s if inverted).

        // Check polarity (Text is usually the minority)
        const size_t total_pixels = small.size();
        const uint* ptr = small.data();
        long count_0 = 0;
        long count_255 = 0;

#pragma omp parallel for reduction(+ : count_0, count_255)
        for (size_t i = 0; i < total_pixels; ++i)
        {
            if (ptr[i] < 128)
                count_0++;
            else
                count_255++;
        }

        // If 0s are minority, assume 0 is text. Otherwise assume 255 is text (inverted image).
        uint text_val = (count_0 < count_255) ? 0 : 255;

        std::vector<Point> points;
        points.reserve(total_pixels / 10);

        for (int y = 0; y < new_h; ++y)
        {
            const uint* row_ptr = small.data(0, y);
            for (int x = 0; x < new_w; ++x)
            {
                if (row_ptr[x] == text_val)
                {
                    points.push_back({x, y});
                }
            }
        }

        if (points.empty())
            return 0.0;

        // Coarse-to-fine Search (Radon)
        // Identical to before - using the points list is still fast regardless of how we got them.
        double base_score = score_angle_radon(points, new_w, new_h, 0.0);

        auto [a1, s1] = search_best_angle_radon(points, new_w, new_h, -15.0, 15.0, 1.0);
        auto [a2, s2] = search_best_angle_radon(points, new_w, new_h, a1 - 1.0, a1 + 1.0, 0.2);
        auto [a3, s3] = search_best_angle_radon(points, new_w, new_h, a2 - 0.3, a2 + 0.3, 0.05);

        double best_angle = a3;
        double best_score = s3;

        if (best_score < base_score * 1.005)
        {
            return 0.0;
        }

        return best_angle;
    }

    void apply_deskew(CImg<uint> &input_image, double angle, int boundary_conditions)
    {
        if (std::abs(angle) > 0.05)
        {
            input_image.rotate(-angle, 2, boundary_conditions); // 2 = Linear Interpolation
        }
    }

    void deskew_projection_profile(CImg<uint> &input_image, int boundary_conditions, const int window_size, const float k, const float delta)
    {
        double angle = detect_skew_angle(input_image, window_size, k, delta);
        apply_deskew(input_image, angle, boundary_conditions);
    }

} // namespace ite::geometry
