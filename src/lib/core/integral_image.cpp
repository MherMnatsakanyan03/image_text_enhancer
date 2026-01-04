#include "integral_image.h"

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

} // namespace ite::core
