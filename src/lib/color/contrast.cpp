#include "contrast.h"


namespace ite::color
{

    void contrast_linear_stretch(CImg<uint> &input_image)
    {
        if (input_image.is_empty())
        {
            return;
        }

        // Compute Histogram (to find percentiles)
        const CImg<uint> hist = input_image.get_histogram(256, 0, 255);
        const uint total_pixels = input_image.size();

        // Find lower (1%) and upper (99%) cutoffs
        const uint cutoff = total_pixels / 100; // 1% threshold

        uint min_val = 0;
        uint count = 0;
        // Find the brightest pixel above cutoff
        for (int i = 0; i < 256; ++i)
        {
            count += hist[i];
            if (count > cutoff)
            {
                min_val = i;
                break;
            }
        }

        // Find the darkest pixel below cutoff
        uint max_val = 255;
        count = 0;
        for (int i = 255; i >= 0; --i)
        {
            count += hist[i];
            if (count > cutoff)
            {
                max_val = i;
                break;
            }
        }

        // Safety check: if image is solid color, min might equal max
        if (max_val <= min_val)
        {
            return;
        }

        // Apply the stretch
        // Formula: 255 * (val - min) / (max - min)
        const float scale = 255.0f / (max_val - min_val);

#pragma omp parallel for
        for (uint i = 0; i < total_pixels; ++i)
        {
            uint val = input_image[i];

            if (val <= min_val)
            {
                input_image[i] = 0;
            }
            else if (val >= max_val)
            {
                input_image[i] = 255;
            }
            else
            {
                input_image[i] = static_cast<uint>((val - min_val) * scale);
            }
        }
    }


} // namespace ite::color
