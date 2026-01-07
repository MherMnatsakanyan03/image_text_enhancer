#include "filters.h"

#include <array>
#include <cstdint>
#include <vector>

#include "core/utils.h"


namespace ite::filters
{

    void simple_gaussian_blur(CImg<uint> &image, float sigma, int boundary_conditions)
    {
        // CImg blur: sigma, boundary_conditions (0=Dirichlet, 1=Neumann, ...), is_gaussian (true)
        image.blur(sigma, boundary_conditions, true);
    }

    // ================= Adaptive Gaussian blur (edge-adaptive blend of two Gaussians) =================
    // Idea: compute a low-sigma blur (preserve edges) and a high-sigma blur (smooth flats),
    // then blend per-pixel using an edge strength measure (fast gradient).
    // The border condition is "replicate".
    // Space/time efficient: 1 extra full image (high blur) + per-thread row-block buffer; 2 separable blurs + 1 blend pass.
    // Parallel: OpenMP over (channel, depth, row-block).

    // In-place adaptive Gaussian blur
    void adaptive_gaussian_blur_omp(CImg<uint> &img, float sigma_low, float sigma_high,
                                    float edge_thresh, // gradient threshold controlling blend (typical 30..80 for 8-bit)
                                    int block_h, int boundary_conditions)
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
                simple_gaussian_blur(img, sigma_low, boundary_conditions);
            return;
        }

        // If no real adaptation requested, fall back to regular blur
        if (!(sigma_high > sigma_low) || sigma_high <= 0.0f)
        {
            if (sigma_low > 0.0f)
                simple_gaussian_blur(img, sigma_low, boundary_conditions);
            return;
        }

        if (block_h < 8)
            block_h = 8;

        // 1) Compute the high-sigma blur into a single extra image
        CImg<uint> high = img;
        simple_gaussian_blur(high, sigma_high, boundary_conditions);

        // 2) Compute the low-sigma blur in-place (img becomes "low")
        if (sigma_low > 0.0f)
            simple_gaussian_blur(img, sigma_low, boundary_conditions);

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

    // ===================== Noise / edge estimators (parallel + histogram based) =====================

    float estimate_noise_sigma_mad_diffs_omp(const CImg<uint> &gray, int step = 2)
    {
        // Robust sigma estimate from median absolute differences (horizontal+vertical).
        // For Gaussian noise: median(|diff|) ≈ 0.6745 * sigma * sqrt(2)
        const int w = gray.width(), h = gray.height();
        if (w < 2 || h < 2)
            return 0.0f;
        if (step < 1)
            step = 1;

        std::array<uint64_t, 256> hist{}; // global

#pragma omp parallel
        {
            std::array<uint64_t, 256> local{};
#pragma omp for schedule(static)
            for (int y = 0; y < h; y += step)
            {
                const uint* row = gray.data(0, y, 0, 0);
                for (int x = 0; x < w - 1; x += step)
                {
                    uint a = row[x], b = row[x + 1];
                    uint d = (a > b) ? (a - b) : (b - a);
                    local[(uint8_t)d]++;
                }
            }
#pragma omp for schedule(static)
            for (int y = 0; y < h - 1; y += step)
            {
                const uint* row = gray.data(0, y, 0, 0);
                const uint* row2 = gray.data(0, y + 1, 0, 0);
                for (int x = 0; x < w; x += step)
                {
                    uint a = row[x], b = row2[x];
                    uint d = (a > b) ? (a - b) : (b - a);
                    local[(uint8_t)d]++;
                }
            }
#pragma omp critical
            {
                for (int i = 0; i < 256; ++i)
                    hist[i] += local[i];
            }
        }

        uint64_t total = 0;
        for (int i = 0; i < 256; ++i)
            total += hist[i];
        if (total == 0)
            return 0.0f;

        const uint64_t target = (total + 1) / 2;
        uint64_t cum = 0;
        int med = 0;
        for (; med < 256; ++med)
        {
            cum += hist[med];
            if (cum >= target)
                break;
        }

        const float denom = 0.6745f * std::sqrt(2.0f);
        return (denom > 0.0f) ? ((float)med / denom) : 0.0f;
    }

    float estimate_gradient_percentile_omp(const CImg<uint> &gray, float pct = 0.75f, int step = 2)
    {
        // Gradient magnitude approx = |dx| + |dy| in [0..510]. Histogram percentile.
        const int w = gray.width(), h = gray.height();
        if (w < 2 || h < 2)
            return 0.0f;
        if (step < 1)
            step = 1;
        pct = utils::clampf(pct, 0.0f, 1.0f);

        constexpr int GMAX = 510;
        std::array<uint64_t, GMAX + 1> hist{};

#pragma omp parallel
        {
            std::array<uint64_t, GMAX + 1> local{};
#pragma omp for schedule(static)
            for (int y = 0; y < h - 1; y += step)
            {
                const uint* row = gray.data(0, y, 0, 0);
                const uint* row2 = gray.data(0, y + 1, 0, 0);
                for (int x = 0; x < w - 1; x += step)
                {
                    uint a = row[x];
                    uint dx = (a > row[x + 1]) ? (a - row[x + 1]) : (row[x + 1] - a);
                    uint dy = (a > row2[x]) ? (a - row2[x]) : (row2[x] - a);
                    uint g = dx + dy;
                    if (g > (uint)GMAX)
                        g = (uint)GMAX;
                    local[g]++;
                }
            }
#pragma omp critical
            {
                for (int i = 0; i <= GMAX; ++i)
                    hist[i] += local[i];
            }
        }

        uint64_t total = 0;
        for (int i = 0; i <= GMAX; ++i)
            total += hist[i];
        if (total == 0)
            return 0.0f;

        const uint64_t target = (uint64_t)std::ceil(pct * (double)total);
        uint64_t cum = 0;
        int val = 0;
        for (; val <= GMAX; ++val)
        {
            cum += hist[val];
            if (cum >= target)
                break;
        }
        return (float)val;
    }

    // ===================== Choose sigmas (good defaults for text enhancement) =====================
    // These are heuristics designed to:
    //   - keep sigma_low small to preserve stroke edges,
    //   - increase sigma_high with noise to smooth background/texture,
    //   - set edge_thresh from gradient distribution so edges stay sharp.


    AdaptiveGaussianParams choose_sigmas_for_text_enhancement(const CImg<uint> &gray)
    {
        // Require grayscale 1-channel image.
        const float noise = estimate_noise_sigma_mad_diffs_omp(gray, 2);
        const float g75 = estimate_gradient_percentile_omp(gray, 0.75f, 2);
        const float g90 = estimate_gradient_percentile_omp(gray, 0.90f, 2);

        // Base sigmas from noise (bounded to stay text-friendly)
        float sigma_low = utils::clampf(0.45f + 0.030f * noise, 0.50f, 1.25f);
        float sigma_high = utils::clampf(1.10f + 0.060f * noise, 1.10f, 2.80f);

        // If image is already blurry (weak strong-grad), be more conservative
        if (g90 < 70.0f)
        {
            sigma_low *= 0.85f;
            sigma_high *= 0.85f;
        }

        // Edge threshold for adaptive blend: tie to gradient distribution.
        // Higher => more pixels treated as "flat" (more high-sigma smoothing).
        float edge_thresh = utils::clampf(std::max(25.0f, 0.90f * g75), 25.0f, 160.0f);

        return {sigma_low, sigma_high, edge_thresh};
    }

    void simple_median_blur(CImg<uint> &image, int kernel_size, unsigned int threshold) { image.blur_median(kernel_size, threshold); }

} // namespace ite::filters
