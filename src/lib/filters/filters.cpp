#include "filters.h"

#include <vector>

#include "core/utils.h"


namespace ite::filters
{

    void gaussian_blur(CImg<uint> &image, float sigma, int boundary_conditions)
    {
        // CImg blur: sigma, boundary_conditions (0=Dirichlet, 1=Neumann, ...), is_gaussian (true)
        image.blur(sigma, boundary_conditions, true);
    }

    // ================= Adaptive Gaussian blur (edge-adaptive blend of two Gaussians) =================
    // Idea: compute a low-sigma blur (preserve edges) and a high-sigma blur (smooth flats),
    // then blend per-pixel using an edge strength measure (fast gradient).
    // Space/time efficient: 1 extra full image (high blur) + per-thread row-block buffer; 2 separable blurs + 1 blend pass.
    // Parallel: OpenMP over (channel, depth, row-block).

    // In-place adaptive Gaussian blur
    void adaptive_gaussian_blur_omp(CImg<uint> &img, float sigma_low, float sigma_high,
                                    float edge_thresh, // gradient threshold controlling blend (typical 30..80 for 8-bit)
                                    int truncate = 3, int block_h = 64)
    {
        if (img.is_empty())
            return;

        const int w = img.width();
        const int h = img.height();
        const int d = img.depth();
        const int s = img.spectrum();

        if (w <= 1 || h <= 1)
        {
            // degenerate: just do a normal blur (or nothing)
            if (sigma_low > 0.0f)
                gaussian_blur(img, sigma_low);
            return;
        }

        // If no real adaptation requested, fall back to regular blur
        if (!(sigma_high > sigma_low) || sigma_high <= 0.0f)
        {
            if (sigma_low > 0.0f)
                gaussian_blur(img, sigma_low);
            return;
        }

        if (block_h < 8)
            block_h = 8;

        // 1) Compute the high-sigma blur into a single extra image
        CImg<uint> high = img;
        gaussian_blur(high, sigma_high, truncate);

        // 2) Compute the low-sigma blur in-place (img becomes "low")
        if (sigma_low > 0.0f)
            gaussian_blur(img, sigma_low, truncate);

        // 3) Blend using edge strength from the low-blur image (row-block buffering => safe in-place write)
        const float invT = (edge_thresh > 1e-6f) ? (1.0f / edge_thresh) : 0.0f;

#pragma omp parallel for collapse(3) schedule(static) default(none) shared(img, high, s, d, h, w, invT, block_h)
        for (int c = 0; c < s; ++c)
        {
            for (int z = 0; z < d; ++z)
            {
                for (int y0 = 0; y0 < h; y0 += block_h)
                {
                    const int y1 = std::min(y0 + block_h, h);
                    const int halo_h = (y1 - y0) + 2; // +2 for y-1 and y+1 (r=1)
                    std::vector<uint> lowbuf((size_t)halo_h * w);

                    uint* low_base = img.data(0, 0, z, c);
                    const uint* hi_base = high.data(0, 0, z, c);

                    // Copy low-blur block + halo rows into thread-local buffer (replicate boundary)
                    for (int yy = 0; yy < halo_h; ++yy)
                    {
                        const int ys = utils::clampi(y0 + yy - 1, 0, h - 1);
                        const uint* src = low_base + (size_t)ys * w;
                        uint* dst = lowbuf.data() + (size_t)yy * w;
                        std::copy(src, src + w, dst);
                    }

                    // Blend and write back into img
                    for (int y = y0; y < y1; ++y)
                    {
                        const int by = (y - y0) + 1; // offset into lowbuf (accounts for halo)
                        const uint* r_up = lowbuf.data() + (size_t)(by - 1) * w;
                        const uint* r_mid = lowbuf.data() + (size_t)(by)*w;
                        const uint* r_down = lowbuf.data() + (size_t)(by + 1) * w;

                        uint* out = low_base + (size_t)y * w;
                        const uint* hi = hi_base + (size_t)y * w;

                        // x = 0 (replicate left)
                        {
                            const int dx = (int)r_mid[1] - (int)r_mid[0];
                            const int dy = (int)r_down[0] - (int)r_up[0];
                            auto grad = (float)(std::abs(dx) + std::abs(dy)); // fast L1 magnitude

                            float t = invT > 0.0f ? utils::clampf(grad * invT, 0.0f, 1.0f) : 1.0f;
                            // smoothstep for stable blending
                            float a = t * t * (3.0f - 2.0f * t); // a=1 -> prefer low (edges), a=0 -> prefer high (flats)

                            auto lv = (float)r_mid[0];
                            auto hv = (float)hi[0];
                            out[0] = utils::clamp_float_to_u8(a * lv + (1.0f - a) * hv);
                        }

                        // center
                        for (int x = 1; x < w - 1; ++x)
                        {
                            const int dx = (int)r_mid[x + 1] - (int)r_mid[x - 1];
                            const int dy = (int)r_down[x] - (int)r_up[x];
                            auto grad = (float)(std::abs(dx) + std::abs(dy));

                            float t = invT > 0.0f ? utils::clampf(grad * invT, 0.0f, 1.0f) : 1.0f;
                            float a = t * t * (3.0f - 2.0f * t);

                            auto lv = (float)r_mid[x];
                            auto hv = (float)hi[x];
                            out[x] = utils::clamp_float_to_u8(a * lv + (1.0f - a) * hv);
                        }

                        // x = w-1 (replicate right)
                        {
                            const int xm1 = w - 2;
                            const int x = w - 1;
                            const int dx = (int)r_mid[x] - (int)r_mid[xm1];
                            const int dy = (int)r_down[x] - (int)r_up[x];
                            auto grad = (float)(std::abs(dx) + std::abs(dy));

                            float t = invT > 0.0f ? utils::clampf(grad * invT, 0.0f, 1.0f) : 1.0f;
                            float a = t * t * (3.0f - 2.0f * t);

                            auto lv = (float)r_mid[x];
                            auto hv = (float)hi[x];
                            out[x] = utils::clamp_float_to_u8(a * lv + (1.0f - a) * hv);
                        }
                    }
                }
            }
        }
    }

} // namespace ite::filters
