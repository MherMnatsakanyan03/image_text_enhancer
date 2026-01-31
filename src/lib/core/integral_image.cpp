#include "integral_image.h"
#include <algorithm>
#include <vector>

namespace ite::core
{

    CImg<double> calculate_integral_image(const CImg<double> &src)
    {
        CImg<double> integral_img(src.width(), src.height(), src.depth(), src.spectrum(), 0);

        cimg_forZC(src, z, c)
        {
            // Process each 2D slice
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

    double get_area_sum(const CImg<double> &integral_img, int x1, int y1, int z, int c, int x2, int y2)
    {
        // Get sum at all four corners of the rectangle, handling boundary conditions
        const double D = integral_img(x2, y2, z, c);
        const double B = (x1 > 0) ? integral_img(x1 - 1, y2, z, c) : 0.0;
        const double C = (y1 > 0) ? integral_img(x2, y1 - 1, z, c) : 0.0;
        const double A = (x1 > 0 && y1 > 0) ? integral_img(x1 - 1, y1 - 1, z, c) : 0.0;

        // Inclusion-Exclusion Principle
        return D - B - C + A;
    }

    void compute_fused_integrals(const CImg<uint> &src, int z, std::vector<double> &S, std::vector<double> &S2)
    {
        const int w = src.width();
        const int h = src.height();
        const int stride = w + 1;

        // Resize and Zero-init
        if (S.size() != (size_t)(stride * (h + 1)))
        {
            S.assign(stride * (h + 1), 0.0);
            S2.assign(stride * (h + 1), 0.0);
        }
        else
        {
            // Technically only need to zero first row/col, but fill is safe/fast
            std::fill(S.begin(), S.end(), 0.0);
            std::fill(S2.begin(), S2.end(), 0.0);
        }

        // Pass 1: Row Accumulation
        // We use OpenMP here because building the rows is independent
#pragma omp parallel for
        for (int y = 0; y < h; ++y)
        {
            double row_sum = 0.0;
            double row_sq_sum = 0.0;

            const uint* row_src = src.data(0, y, z);
            double* row_S = S.data() + (size_t)(y + 1) * stride;
            double* row_S2 = S2.data() + (size_t)(y + 1) * stride;

            for (int x = 0; x < w; ++x)
            {
                double val = (double)row_src[x];
                row_sum += val;
                row_sq_sum += val * val;

                // Write 1-based index
                row_S[x + 1] = row_sum;
                row_S2[x + 1] = row_sq_sum;
            }
        }

        // Pass 2: Column Accumulation
        // Dependent on previous row, but columns are independent
#pragma omp parallel for
        for (int x = 1; x <= w; ++x)
        {
            for (int y = 1; y <= h; ++y)
            {
                int curr = y * stride + x;
                int prev = (y - 1) * stride + x;
                S[curr] += S[prev];
                S2[curr] += S2[prev];
            }
        }
    }
} // namespace ite::core
