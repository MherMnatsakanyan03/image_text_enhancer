#include "geometry.h"
#include <algorithm>
#include <cmath>
#include <utility>
#include "../binarization/binarization.h"
#include "../core/utils.h"

using namespace ite::utils;


namespace ite::geometry
{

    // Score: variance of horizontal projection profile
    static double score_angle_variance(const CImg<unsigned char> &bin, int roi_x0, int roi_y0, int roi_w, int roi_h, double angle_deg)
    {
        CImg<unsigned char> rot = bin.get_rotate(angle_deg, 0, 0);

        const int W = rot.width();
        const unsigned char* p = rot.data();

        double sum = 0.0;
        double sum_sq = 0.0;

        for (int y = roi_y0; y < roi_y0 + roi_h; ++y)
        {
            const unsigned char* row = p + y * W + roi_x0;
            int row_sum = 0;
            for (int x = 0; x < roi_w; ++x)
                row_sum += row[x];

            sum += static_cast<double>(row_sum);
            sum_sq += static_cast<double>(row_sum) * static_cast<double>(row_sum);
        }

        const double invH = 1.0 / static_cast<double>(roi_h);
        const double mean = sum * invH;
        return sum_sq * invH - mean * mean;
    }

    static std::pair<double, double> search_best_angle(const CImg<unsigned char> &bin, int roi_x0, int roi_y0, int roi_w, int roi_h, double start_deg,
                                                       double end_deg, double step_deg)
    {
        if (step_deg <= 0.0)
            return {0.0, -1.0};
        if (end_deg < start_deg)
            std::swap(start_deg, end_deg);

        const int N = static_cast<int>(std::floor((end_deg - start_deg) / step_deg)) + 1;
        double best_angle = 0.0;
        double best_score = -1.0;

#pragma omp parallel
        {
            double local_best_a = 0.0;
            double local_best_s = -1.0;

#pragma omp for schedule(static) nowait
            for (int i = 0; i < N; ++i)
            {
                const double a = start_deg + static_cast<double>(i) * step_deg;
                const double s = score_angle_variance(bin, roi_x0, roi_y0, roi_w, roi_h, a);
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

    void deskew_projection_profile(CImg<uint> &input_image, int boundary_conditions, int window_size, float k, float delta)
    {
        const int inW = input_image.width();
        const int inH = input_image.height();
        if (inW <= 1 || inH <= 1)
            return;

        // Downscale for speed
        constexpr double target_long = 600.0;
        const int long_side = std::max(inW, inH);
        double scale = target_long / static_cast<double>(long_side);
        if (scale > 1.0)
            scale = 1.0;

        int new_w = std::max(1, static_cast<int>(std::lround(inW * scale)));
        int new_h = std::max(1, static_cast<int>(std::lround(inH * scale)));

        CImg<uint> small = input_image.get_resize(new_w, new_h, 1, input_image.spectrum());

        // Convert to grayscale (as CImg<uint> for Sauvola)
        CImg<uint> gray(new_w, new_h, 1, 1);
        if (small.spectrum() >= 3)
        {
            for (int y = 0; y < new_h; ++y)
            {
                for (int x = 0; x < new_w; ++x)
                {
                    const uint r = small(x, y, 0, 0);
                    const uint g = small(x, y, 0, 1);
                    const uint b = small(x, y, 0, 2);
                    const double yv = 0.2126 * static_cast<double>(r) + 0.7152 * static_cast<double>(g) + 0.0722 * static_cast<double>(b);
                    gray(x, y) = static_cast<unsigned char>(std::clamp(static_cast<int>(std::lround(yv)), 0, 255));
                }
            }
        }
        else
        {
            for (int y = 0; y < new_h; ++y)
                for (int x = 0; x < new_w; ++x)
                    gray(x, y) = clamp_to_u8(small(x, y, 0, 0));
        }

        // Binarize with Sauvola
        binarization::binarize_sauvola(gray, window_size, k, delta);

        // Build binary image (Sauvola produces 0 or 255, convert to 0 or 1)
        CImg<unsigned char> bin(new_w, new_h, 1, 1);
        {
            const int N = new_w * new_h;
            const uint* pg = gray.data();
            unsigned char* pb = bin.data();

            int fg = 0;
            for (int i = 0; i < N; ++i)
            {
                const unsigned char b = (pg[i] > 0) ? 1u : 0u;
                pb[i] = b;
                fg += b;
            }

            // If foreground > 50%, invert (ensure foreground is minority)
            if (fg > N / 2)
            {
                for (int i = 0; i < N; ++i)
                    pb[i] = static_cast<unsigned char>(1u - pb[i]);
            }
        }

        // Crop to content
        int minx = new_w, miny = new_h, maxx = -1, maxy = -1;
        {
            const unsigned char* pb = bin.data();
            for (int y = 0; y < new_h; ++y)
            {
                const unsigned char* row = pb + y * new_w;
                for (int x = 0; x < new_w; ++x)
                {
                    if (row[x])
                    {
                        minx = std::min(minx, x);
                        miny = std::min(miny, y);
                        maxx = std::max(maxx, x);
                        maxy = std::max(maxy, y);
                    }
                }
            }
        }

        if (maxx < 0 || maxy < 0)
            return;

        const int margin = std::max(2, static_cast<int>(std::lround(0.02 * std::min(new_w, new_h))));
        minx = std::max(0, minx - margin);
        miny = std::max(0, miny - margin);
        maxx = std::min(new_w - 1, maxx + margin);
        maxy = std::min(new_h - 1, maxy + margin);

        CImg<unsigned char> work = bin.get_crop(minx, miny, maxx, maxy);
        const int W = work.width();
        const int H = work.height();
        if (W <= 8 || H <= 8)
            return;

        // Central ROI
        const double pad = 0.10;
        const int roi_w = std::max(1, static_cast<int>(std::lround(W * (1.0 - 2.0 * pad))));
        const int roi_h = std::max(1, static_cast<int>(std::lround(H * (1.0 - 2.0 * pad))));
        const int roi_x0 = (W - roi_w) / 2;
        const int roi_y0 = (H - roi_h) / 2;

        const double base_score = score_angle_variance(work, roi_x0, roi_y0, roi_w, roi_h, 0.0);

        // Coarse-to-fine search
        auto [a1, s1] = search_best_angle(work, roi_x0, roi_y0, roi_w, roi_h, -15.0, 15.0, 1.0);
        auto [a2, s2] = search_best_angle(work, roi_x0, roi_y0, roi_w, roi_h, a1 - 1.0, a1 + 1.0, 0.2);
        auto [a3, s3] = search_best_angle(work, roi_x0, roi_y0, roi_w, roi_h, a2 - 0.3, a2 + 0.3, 0.05);

        const double best_angle = a3;
        const double best_score = s3;

        const double abs_a = std::abs(best_angle);
        const bool angle_ok = (abs_a > 0.05);
        const bool improve_ok = (best_score > base_score + 1e-9) && (base_score <= 0.0 ? true : (best_score >= base_score * 1.002));

        if (angle_ok && improve_ok)
        {
            input_image.rotate(best_angle, 2, boundary_conditions);
        }
    }

    double detect_skew_angle_projection_profile(const CImg<uint> &image)
    {
        // Simplified version that just returns the angle
        CImg<uint> temp = image;

        const int inW = temp.width();
        const int inH = temp.height();
        if (inW <= 1 || inH <= 1)
            return 0.0;

        constexpr double target_long = 600.0;
        const int long_side = std::max(inW, inH);
        double scale = target_long / static_cast<double>(long_side);
        if (scale > 1.0)
            scale = 1.0;

        int new_w = std::max(1, static_cast<int>(std::lround(inW * scale)));
        int new_h = std::max(1, static_cast<int>(std::lround(inH * scale)));

        CImg<uint> small = temp.get_resize(new_w, new_h, 1, temp.spectrum());

        CImg<unsigned char> gray(new_w, new_h, 1, 1);
        if (small.spectrum() >= 3)
        {
            for (int y = 0; y < new_h; ++y)
            {
                for (int x = 0; x < new_w; ++x)
                {
                    const uint r = small(x, y, 0, 0);
                    const uint g = small(x, y, 0, 1);
                    const uint b = small(x, y, 0, 2);
                    const double yv = 0.2126 * r + 0.7152 * g + 0.0722 * b;
                    gray(x, y) = static_cast<unsigned char>(std::clamp(static_cast<int>(std::lround(yv)), 0, 255));
                }
            }
        }
        else
        {
            for (int y = 0; y < new_h; ++y)
                for (int x = 0; x < new_w; ++x)
                    gray(x, y) = clamp_to_u8(small(x, y, 0, 0));
        }

        const int t = binarization::compute_otsu_threshold(gray);
        const double bmean = binarization::compute_border_mean(gray);
        const bool background_is_bright = (bmean >= static_cast<double>(t));

        CImg<unsigned char> bin(new_w, new_h, 1, 1);
        {
            const int N = new_w * new_h;
            const unsigned char* pg = gray.data();
            unsigned char* pb = bin.data();

            int fg = 0;
            if (background_is_bright)
            {
                for (int i = 0; i < N; ++i)
                {
                    pb[i] = (pg[i] <= static_cast<unsigned char>(t)) ? 1u : 0u;
                    fg += pb[i];
                }
            }
            else
            {
                for (int i = 0; i < N; ++i)
                {
                    pb[i] = (pg[i] > static_cast<unsigned char>(t)) ? 1u : 0u;
                    fg += pb[i];
                }
            }

            if (fg > N / 2)
            {
                for (int i = 0; i < N; ++i)
                    pb[i] = 1u - pb[i];
            }
        }

        int minx = new_w, miny = new_h, maxx = -1, maxy = -1;
        {
            const unsigned char* pb = bin.data();
            for (int y = 0; y < new_h; ++y)
            {
                const unsigned char* row = pb + y * new_w;
                for (int x = 0; x < new_w; ++x)
                {
                    if (row[x])
                    {
                        minx = std::min(minx, x);
                        miny = std::min(miny, y);
                        maxx = std::max(maxx, x);
                        maxy = std::max(maxy, y);
                    }
                }
            }
        }

        if (maxx < 0 || maxy < 0)
            return 0.0;

        const int margin = std::max(2, static_cast<int>(std::lround(0.02 * std::min(new_w, new_h))));
        minx = std::max(0, minx - margin);
        miny = std::max(0, miny - margin);
        maxx = std::min(new_w - 1, maxx + margin);
        maxy = std::min(new_h - 1, maxy + margin);

        CImg<unsigned char> work = bin.get_crop(minx, miny, maxx, maxy);
        const int W = work.width();
        const int H = work.height();
        if (W <= 8 || H <= 8)
            return 0.0;

        const double pad = 0.10;
        const int roi_w = std::max(1, static_cast<int>(std::lround(W * (1.0 - 2.0 * pad))));
        const int roi_h = std::max(1, static_cast<int>(std::lround(H * (1.0 - 2.0 * pad))));
        const int roi_x0 = (W - roi_w) / 2;
        const int roi_y0 = (H - roi_h) / 2;

        auto [a1, s1] = search_best_angle(work, roi_x0, roi_y0, roi_w, roi_h, -15.0, 15.0, 1.0);
        auto [a2, s2] = search_best_angle(work, roi_x0, roi_y0, roi_w, roi_h, a1 - 1.0, a1 + 1.0, 0.2);
        auto [a3, s3] = search_best_angle(work, roi_x0, roi_y0, roi_w, roi_h, a2 - 0.3, a2 + 0.3, 0.05);

        return a3;
    }

} // namespace ite::geometry
