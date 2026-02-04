#include "morphology.h"
#include <algorithm>
#include <omp.h>
#include <stdexcept>
#include <vector>

namespace ite::morphology
{

    // Helper: 1D Sliding Window Maximum (Dilation) using Monotonic Queue
    static void sliding_window_max(const uint* src, uint* dst, int len, int stride, int r)
    {
        // We use a vector as a raw ring-buffer/deque for maximum performance
        std::vector<int> q(len);
        int head = 0;
        int tail = 0;

        // The window size is 2*r + 1.
        // We iterate 'i' as the right edge of the sliding window.
        // The output is written to the center index (i - r).
        for (int i = 0; i < len + r; ++i)
        {
            // 1. Remove index from front if it fell out of the window
            // The valid window range for center (i-r) ends at i.
            // The element falling out is (i - (2*r + 1)).
            if (head < tail && q[head] <= i - (2 * r + 1))
            {
                head++;
            }

            // 2. Add current element 'i' to the queue (if within bounds)
            if (i < len)
            {
                uint val = src[i * stride];
                // Maintain Decreasing Order: Pop back elements smaller than current
                while (head < tail && src[q[tail - 1] * stride] <= val)
                {
                    tail--;
                }
                q[tail++] = i;
            }

            // 3. Set the max value for the center pixel
            int center = i - r;
            if (center >= 0 && center < len)
            {
                // The front of the queue is always the max in the current window
                if (head < tail)
                {
                    dst[center * stride] = src[q[head] * stride];
                }
            }
        }
    }

    // Helper: 1D Sliding Window Minimum (Erosion) using Monotonic Queue
    static void sliding_window_min(const uint* src, uint* dst, int len, int stride, int r)
    {
        std::vector<int> q(len);
        int head = 0;
        int tail = 0;

        for (int i = 0; i < len + r; ++i)
        {
            // 1. Remove expired indices
            if (head < tail && q[head] <= i - (2 * r + 1))
            {
                head++;
            }

            // 2. Add current element
            if (i < len)
            {
                uint val = src[i * stride];
                // Maintain Increasing Order: Pop back elements larger than current
                while (head < tail && src[q[tail - 1] * stride] >= val)
                {
                    tail--;
                }
                q[tail++] = i;
            }

            // 3. Set output
            int center = i - r;
            if (center >= 0 && center < len)
            {
                if (head < tail)
                {
                    dst[center * stride] = src[q[head] * stride];
                }
            }
        }
    }

    void dilation_square(CImg<uint> &input_image, int kernel_size)
    {
        if (input_image.spectrum() != 1)
        {
            throw std::runtime_error("Dilation requires a single-channel image.");
        }

        if (kernel_size <= 1)
        {
            return;
        }

        int r = kernel_size / 2;
        int w = input_image.width();
        int h = input_image.height();
        int d = input_image.depth();

        // Temporary buffer is required for separable filters
        CImg<uint> temp = input_image;

        // Pass 1: Horizontal Dilation (Row by Row)
        // Reads input_image, Writes to temp
#pragma omp parallel for collapse(2)
        for (int z = 0; z < d; ++z)
        {
            for (int y = 0; y < h; ++y)
            {
                const uint* src_row = input_image.data(0, y, z);
                uint* dst_row = temp.data(0, y, z);
                // Stride 1: contiguous memory
                sliding_window_max(src_row, dst_row, w, 1, r);
            }
        }

        // Pass 2: Vertical Dilation (Col by Col)
        // Reads temp, Writes to input_image
#pragma omp parallel for collapse(2)
        for (int z = 0; z < d; ++z)
        {
            for (int x = 0; x < w; ++x)
            {
                const uint* src_col = temp.data(x, 0, z);
                uint* dst_col = input_image.data(x, 0, z);
                // Stride w: vertical step
                sliding_window_max(src_col, dst_col, h, w, r);
            }
        }
    }

    void erosion_square(CImg<uint> &input_image, int kernel_size)
    {
        if (input_image.spectrum() != 1)
        {
            throw std::runtime_error("Erosion requires a single-channel image.");
        }

        if (kernel_size <= 1)
        {
            return;
        }

        int r = kernel_size / 2;
        int w = input_image.width();
        int h = input_image.height();
        int d = input_image.depth();

        CImg<uint> temp = input_image;

        // Pass 1: Horizontal Erosion
#pragma omp parallel for collapse(2)
        for (int z = 0; z < d; ++z)
        {
            for (int y = 0; y < h; ++y)
            {
                const uint* src_row = input_image.data(0, y, z);
                uint* dst_row = temp.data(0, y, z);
                sliding_window_min(src_row, dst_row, w, 1, r);
            }
        }

        // Pass 2: Vertical Erosion
#pragma omp parallel for collapse(2)
        for (int z = 0; z < d; ++z)
        {
            for (int x = 0; x < w; ++x)
            {
                const uint* src_col = temp.data(x, 0, z);
                uint* dst_col = input_image.data(x, 0, z);
                sliding_window_min(src_col, dst_col, h, w, r);
            }
        }
    }

    void despeckle_ccl(CImg<uint> &input_image, const uint threshold, bool diagonal_connections)
    {
        if (threshold <= 0)
        {
            return;
        }

        // Invert image so Text/Noise becomes White (255) and Background becomes Black (0)
        // CCL works on non-zero values.
#pragma omp parallel for
        for (uint i = 0; i < input_image.size(); ++i)
        {
            input_image[i] = (input_image[i] == 0) ? 255 : 0;
        }

        // Label connected components
        CImg<uint> labels = input_image.get_label(diagonal_connections);

        uint max_label = labels.max();
        if (max_label == 0)
        {
            // Image was fully black (originally white), revert and return
            input_image.fill(255);
            return;
        }

        // Histogram of component sizes
        std::vector<uint> sizes(max_label + 1, 0);
        cimg_for(labels, ptr, uint) { sizes[*ptr]++; }

        // Filter small components
#pragma omp parallel for collapse(2)
        for (int z = 0; z < input_image.depth(); ++z)
        {
            for (int y = 0; y < input_image.height(); ++y)
            {
                for (int x = 0; x < input_image.width(); ++x)
                {
                    uint label_id = labels(x, y, z);
                    // If component size < threshold, turn it off (set to 0 / black)
                    if (label_id > 0 && sizes[label_id] < threshold)
                    {
                        input_image(x, y, z) = 0;
                    }
                }
            }
        }

        // Invert back
#pragma omp parallel for
        for (uint i = 0; i < input_image.size(); ++i)
        {
            input_image[i] = (input_image[i] == 255) ? 0 : 255;
        }
    }

} // namespace ite::morphology
