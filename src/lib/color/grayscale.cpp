#include "grayscale.h"
#include <cmath>

namespace ite::color
{

    void to_grayscale_rec601(CImg<uint> &input_image)
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
                    uint r = input_image(x, y, z, 0);
                    uint g = input_image(x, y, z, 1);
                    uint b = input_image(x, y, z, 2);
                    gray_image(x, y, z, 0) = static_cast<uint>(std::round(WEIGHT_R * r + WEIGHT_G * g + WEIGHT_B * b));
                }
            }
        }

        input_image = gray_image;
    }

} // namespace ite::color
