#include "grayscale.h"
#include <cmath>

namespace ite::color
{

    void to_grayscale_rec601(const CImg<uint> &input, CImg<uint> &output)
    {
        if (input.spectrum() == 1)
        {
            // Already grayscale — copy input directly to output
            output = input;
            return;
        }

        // Resize output to the correct 1-channel dimensions
        output.assign(input.width(), input.height(), input.depth(), 1);

#pragma omp parallel for collapse(2)
        for (int z = 0; z < input.depth(); ++z)
        {
            for (int y = 0; y < input.height(); ++y)
            {
                for (int x = 0; x < input.width(); ++x)
                {
                    uint r = input(x, y, z, 0);
                    uint g = input(x, y, z, 1);
                    uint b = input(x, y, z, 2);
                    output(x, y, z, 0) = static_cast<uint>(std::round(WEIGHT_R * r + WEIGHT_G * g + WEIGHT_B * b));
                }
            }
        }
    }

} // namespace ite::color
