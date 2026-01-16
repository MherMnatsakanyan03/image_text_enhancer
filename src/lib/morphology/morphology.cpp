#include "morphology.h"
#include <stdexcept>
#include <vector>


namespace ite::morphology
{

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

        CImg<uint> source = input_image;

        int r = kernel_size / 2;
        int w = input_image.width();
        int h = input_image.height();
        int d = input_image.depth();

#pragma omp parallel for collapse(2)
        for (int i_d = 0; i_d < d; ++i_d)
        {
            for (int i_h = 0; i_h < h; ++i_h)
            {
                for (int i_w = 0; i_w < w; ++i_w)
                {
                    // If the pixel is already white, it stays white
                    if (source(i_w, i_h, i_d) == 255)
                    {
                        continue;
                    }

                    bool hit = false;

                    // Scan neighborhood
                    for (int k_h = -r; k_h <= r && !hit; ++k_h)
                    {
                        for (int k_w = -r; k_w <= r && !hit; ++k_w)
                        {
                            int n_w = i_w + k_w;
                            int n_h = i_h + k_h;

                            if (n_w >= 0 && n_w < w && n_h >= 0 && n_h < h)
                            {
                                if (source(n_w, n_h, i_d) == 255)
                                {
                                    hit = true;
                                }
                            }
                        }
                    }

                    if (hit)
                    {
                        input_image(i_w, i_h, i_d) = 255;
                    }
                }
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

        CImg<uint> source = input_image;

        int r = kernel_size / 2;
        int w = input_image.width();
        int h = input_image.height();
        int d = input_image.depth();

#pragma omp parallel for collapse(2)
        for (int i_d = 0; i_d < d; ++i_d)
        {
            for (int i_h = 0; i_h < h; ++i_h)
            {
                for (int i_w = 0; i_w < w; ++i_w)
                {
                    // If the pixel is already black, it stays black
                    if (source(i_w, i_h, i_d) == 0)
                    {
                        continue;
                    }

                    bool hit = false;

                    // Scan neighborhood
                    for (int k_h = -r; k_h <= r && !hit; ++k_h)
                    {
                        for (int k_w = -r; k_w <= r && !hit; ++k_w)
                        {
                            int n_w = i_w + k_w;
                            int n_h = i_h + k_h;

                            if (n_w >= 0 && n_w < w && n_h >= 0 && n_h < h)
                            {
                                if (source(n_w, n_h, i_d) == 0)
                                {
                                    hit = true;
                                }
                            }
                        }
                    }

                    if (hit)
                    {
                        input_image(i_w, i_h, i_d) = 0;
                    }
                }
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
            input_image.fill(255);
            return;
        }

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
