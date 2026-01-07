#include "filters.h"

#include <array>
#include <cstdint>
#include <vector>

#include "core/utils.h"


namespace ite::filters
{

    // ================= Gaussian blur =================

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

    // ================= END Gaussian blur =================

    // ===================== Median blur =====================

    void simple_median_blur(CImg<uint> &image, int kernel_size, unsigned int threshold) { image.blur_median(kernel_size, threshold); }


    // --------- Median-of-9 (3x3) fast network ----------
    void pix_sort(uint &a, uint &b)
    {
        if (a > b)
        {
            uint t = a;
            a = b;
            b = t;
        }
    }

    uint median9(uint p0, uint p1, uint p2, uint p3, uint p4, uint p5, uint p6, uint p7, uint p8)
    {
        // Proven correct "opt_med9" style network
        pix_sort(p1, p2);
        pix_sort(p4, p5);
        pix_sort(p7, p8);
        pix_sort(p0, p1);
        pix_sort(p3, p4);
        pix_sort(p6, p7);
        pix_sort(p1, p2);
        pix_sort(p4, p5);
        pix_sort(p7, p8);
        pix_sort(p0, p3);
        pix_sort(p5, p8);
        pix_sort(p4, p7);
        pix_sort(p3, p6);
        pix_sort(p1, p4);
        pix_sort(p2, p5);
        pix_sort(p4, p7);
        pix_sort(p4, p2);
        pix_sort(p6, p4);
        pix_sort(p4, p2);
        return p4; // median
    }

    // ---------- Histogram helpers for adaptive median ----------
    static inline void hist_add(std::array<uint16_t, 256> &hist, std::array<uint8_t, 256> &touched, int &ntouched, uint v)
    {
        const uint8_t b = (uint8_t)v;
        if (hist[b]++ == 0)
            touched[ntouched++] = b;
    }

    static inline void hist_reset(std::array<uint16_t, 256> &hist, const std::array<uint8_t, 256> &touched, int ntouched)
    {
        for (int i = 0; i < ntouched; ++i)
            hist[touched[i]] = 0;
    }

    static inline void hist_min_med_max(const std::array<uint16_t, 256> &hist, int total, uint &zmin, uint &zmed, uint &zmax)
    {
        // min
        int i = 0;
        while (i < 256 && hist[i] == 0)
            ++i;
        zmin = (i < 256) ? (uint)i : 0u;

        // max
        int j = 255;
        while (j >= 0 && hist[j] == 0)
            --j;
        zmax = (j >= 0) ? (uint)j : 255u;

        // median
        const int target = (total + 1) / 2;
        int cum = 0;
        for (int k = 0; k < 256; ++k)
        {
            cum += (int)hist[k];
            if (cum >= target)
            {
                zmed = (uint)k;
                return;
            }
        }
        zmed = zmax;
    }

    /**
     * Adaptive Median Filter (AMF), in-place, OpenMP, space-efficient.
     * - Starts with 3x3. If pixel looks like impulse noise, expands window up to max_window_size (odd).
     * - Great for scan speckle / salt-and-pepper while preserving text edges (often leaves non-impulse pixels unchanged).
     *
     * Params:
     *   max_window_size: odd >=3 (typical 5/7/9)
     *   block_h: row-block height for cache + in-place safety
     */
    void adaptive_median_filter(CImg<uint> &img, int max_window_size = 7, int block_h = 64)
    {
        if (img.is_empty())
            return;

        const int w = img.width();
        const int h = img.height();
        const int d = img.depth();
        const int s = img.spectrum();
        if (w < 2 || h < 2)
            return;

        if (max_window_size < 3)
            max_window_size = 3;
        if ((max_window_size & 1) == 0)
            ++max_window_size;
        const int max_r = (max_window_size - 1) / 2;

        if (block_h < 8)
            block_h = 8;

#pragma omp parallel for collapse(3) schedule(static)
        for (int c = 0; c < s; ++c)
        {
            for (int z = 0; z < d; ++z)
            {
                for (int y0 = 0; y0 < h; y0 += block_h)
                {

                    const int y1 = std::min(y0 + block_h, h);
                    const int halo_h = (y1 - y0) + 2 * max_r;

                    // Copy block + halo rows into thread-local buffer (replicate boundary).
                    std::vector<uint> src((size_t)halo_h * w);
                    uint* base = img.data(0, 0, z, c);
                    for (int yy = 0; yy < halo_h; ++yy)
                    {
                        const int ys = utils::clampi(y0 + yy - max_r, 0, h - 1);
                        const uint* row_src = base + (size_t)ys * w;
                        uint* row_dst = src.data() + (size_t)yy * w;
                        std::copy(row_src, row_src + w, row_dst);
                    }

                    // Per-thread reusable histogram state (no per-pixel allocations).
                    std::array<uint16_t, 256> hist{};
                    std::array<uint8_t, 256> touched{};

                    // Process rows in block; write back into original image.
                    for (int y = y0; y < y1; ++y)
                    {
                        const int by = (y - y0) + max_r; // center row index in src buffer
                        uint* out = base + (size_t)y * w;

                        const uint* r_m1 = src.data() + (size_t)(by - 1) * w;
                        const uint* r_0 = src.data() + (size_t)(by)*w;
                        const uint* r_p1 = src.data() + (size_t)(by + 1) * w;

                        for (int x = 0; x < w; ++x)
                        {
                            const int xm1 = (x == 0) ? 0 : x - 1;
                            const int xp1 = (x == w - 1) ? (w - 1) : x + 1;

                            const uint zxy = r_0[x];

                            // Stage with 3x3 using very fast ops
                            uint p0 = r_m1[xm1], p1 = r_m1[x], p2 = r_m1[xp1];
                            uint p3 = r_0[xm1], p4 = r_0[x], p5 = r_0[xp1];
                            uint p6 = r_p1[xm1], p7 = r_p1[x], p8 = r_p1[xp1];

                            uint zmed = median9(p0, p1, p2, p3, p4, p5, p6, p7, p8);
                            uint zmin = p0, zmax = p0;
                            // min/max of 9 (cheap)
                            uint arr9[9] = {p0, p1, p2, p3, p4, p5, p6, p7, p8};
                            for (int i = 1; i < 9; ++i)
                            {
                                zmin = std::min(zmin, arr9[i]);
                                zmax = std::max(zmax, arr9[i]);
                            }

                            // AMF Stage A / B decision at r=1
                            if (zmed > zmin && zmed < zmax)
                            {
                                // Stage B
                                out[x] = (zxy > zmin && zxy < zmax) ? zxy : zmed;
                                continue;
                            }

                            // Need to expand: initialize histogram with 3x3 (already loaded)
                            int ntouched = 0;
                            hist_add(hist, touched, ntouched, p0);
                            hist_add(hist, touched, ntouched, p1);
                            hist_add(hist, touched, ntouched, p2);
                            hist_add(hist, touched, ntouched, p3);
                            hist_add(hist, touched, ntouched, p4);
                            hist_add(hist, touched, ntouched, p5);
                            hist_add(hist, touched, ntouched, p6);
                            hist_add(hist, touched, ntouched, p7);
                            hist_add(hist, touched, ntouched, p8);

                            int total = 9;
                            uint outv = zmed;

                            // Expand window: r = 2..max_r
                            for (int r = 2; r <= max_r; ++r)
                            {
                                // Add ring pixels (8r pixels) with replicate boundary in x, halo already in y.
                                const int xl = utils::clampi(x - r, 0, w - 1);
                                const int xr = utils::clampi(x + r, 0, w - 1);

                                // Vertical sides for dy in [-r..r]
                                for (int dy = -r; dy <= r; ++dy)
                                {
                                    const uint* row = src.data() + (size_t)(by + dy) * w;
                                    hist_add(hist, touched, ntouched, row[xl]);
                                    hist_add(hist, touched, ntouched, row[xr]);
                                }
                                // Top & bottom (excluding corners) for dx in [-(r-1)..(r-1)]
                                const uint* rowt = src.data() + (size_t)(by - r) * w;
                                const uint* rowb = src.data() + (size_t)(by + r) * w;
                                for (int dx = -(r - 1); dx <= (r - 1); ++dx)
                                {
                                    const int xx = utils::clampi(x + dx, 0, w - 1);
                                    hist_add(hist, touched, ntouched, rowt[xx]);
                                    hist_add(hist, touched, ntouched, rowb[xx]);
                                }

                                total = (2 * r + 1) * (2 * r + 1);
                                hist_min_med_max(hist, total, zmin, zmed, zmax);

                                if (zmed > zmin && zmed < zmax)
                                {
                                    outv = (zxy > zmin && zxy < zmax) ? zxy : zmed;
                                    break;
                                }
                                outv = zmed; // if we hit max_r, this is what we'd output
                            }

                            out[x] = outv;
                            hist_reset(hist, touched, ntouched);
                        }
                    }
                }
            }
        }
    }

    // ===================== END Median blur =====================

} // namespace ite::filters
