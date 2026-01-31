#include "contrast.h"
#include <algorithm>
#include <cstdint>
#include <omp.h>
#include <vector>

namespace ite::color
{
    void contrast_linear_stretch(CImg<uint> &input_image)
    {
        if (input_image.is_empty())
        {
            return;
        }

        const size_t N = input_image.size();
        uint* data_ptr = input_image.data();

        // Parallel Histogram Calculation (No allocations, 64-bit bins for safety)
        uint64_t hist[256] = {0};

#pragma omp parallel
        {
            uint64_t local_hist[256] = {0};

            // Each thread counts its chunk of pixels
#pragma omp for schedule(static)
            for (size_t i = 0; i < N; ++i)
            {
                uint val = data_ptr[i];
                val = std::min(255u, val); // Safety clamp
                local_hist[val]++;
            }

            // Combine local histograms into global
#pragma omp critical
            {
                for (int i = 0; i < 256; ++i)
                {
                    hist[i] += local_hist[i];
                }
            }
        }

        // 2. Find Percentiles (Same logic as before)
        const uint64_t cutoff = N / 100; // 1% threshold

        uint min_val = 0;
        uint64_t count = 0;
        for (int i = 0; i < 256; ++i)
        {
            count += hist[i];
            if (count > cutoff)
            {
                min_val = i;
                break;
            }
        }

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

        if (max_val <= min_val)
        {
            return;
        }

        // OPTIMIZATION: Precompute Lookup Table (LUT)
        // Replaces per-pixel float math and branching with a single array read.
        uint8_t lut[256];
        const float scale = 255.0f / (max_val - min_val);

        for (int i = 0; i < 256; ++i)
        {
            if (i <= (int)min_val)
                lut[i] = 0;
            else if (i >= (int)max_val)
                lut[i] = 255;
            else
                lut[i] = static_cast<uint8_t>((i - min_val) * scale);
        }

        // Apply LUT (Vectorizable & Parallel)
#pragma omp parallel for schedule(static)
        for (size_t i = 0; i < N; ++i)
        {
            // Safety check for values > 255 (though inputs should be 8-bit range)
            uint val = std::min(255u, data_ptr[i]);
            data_ptr[i] = lut[val];
        }
    }
} // namespace ite::color
