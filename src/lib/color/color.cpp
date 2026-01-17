#include "color.h"
#include <stdexcept>


namespace ite::color
{

    void color_pass_inplace(CImg<uint> &color_image, const CImg<uint> &bin_image)
    {
        if (bin_image.is_empty() || color_image.is_empty())
        {
            return;
        }

        if (bin_image.spectrum() != 1)
        {
            throw std::invalid_argument("Binary mask must have a single channel.");
        }
        if (color_image.spectrum() != 3)
        {
            throw std::invalid_argument("Color image must have 3 channels.");
        }
        if (color_image.width() != bin_image.width() || color_image.height() != bin_image.height())
        {
            throw std::invalid_argument("Images must have the same dimensions.");
        }

        int w = color_image.width();
        int h = color_image.height();
        int d = color_image.depth();

// Iterate over the image and apply mask
#pragma omp parallel for collapse(2)
        for (int z = 0; z < d; ++z)
        {
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    // Check the mask pixel (Single channel)
                    // If Mask is White (Background), set output to White.
                    // If Mask is Black (Text), leave the original color alone.
                    if (bin_image(x, y, z) == 255)
                    {
                        color_image(x, y, z, 0) = 255; // R
                        color_image(x, y, z, 1) = 255; // G
                        color_image(x, y, z, 2) = 255; // B
                    }
                }
            }
        }
    }

} // namespace ite::color
