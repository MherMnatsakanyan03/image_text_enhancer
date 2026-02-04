#include "filters.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include "core/utils.h"

namespace ite::filters
{
    // ================= Gaussian blur =================
    void simple_gaussian_blur(CImg<uint> &image, float sigma, int boundary_conditions) { image.blur(sigma, boundary_conditions, true); }

    // ================= Adaptive Gaussian blur =================
    // Idea: compute a low-sigma blur (preserve edges) and a high-sigma blur (smooth flats),
    // then blend per-pixel using an edge strength measure (fast gradient).
    // The border condition is "replicate".
    // Space/time efficient: 1 extra full image (high blur) + per-thread row-block buffer; 2 separable blurs + 1 blend pass.
    // Parallel: OpenMP over (channel, depth, row-block).

    // In-place adaptive Gaussian blur
    void adaptive_gaussian_blur(CImg<uint> &img, float sigma_low, float sigma_high, float edge_thresh, int block_h, int boundary_conditions)
    {
        if (img.is_empty())
            return;

        // Validation
        if (!(sigma_high > sigma_low) || sigma_high <= 0.0f)
        {
            if (sigma_low > 0.0f)
                simple_gaussian_blur(img, sigma_low, boundary_conditions);
            return;
        }

        if (block_h < 8)
            block_h = 8;
        const int w = img.width();
        const int h = img.height();
        const int d = img.depth();
        const int s = img.spectrum();

        // 1. Prepare Buffers
        // 'high' will start as the High-Sigma blur
        // 'img' will become the Low-Sigma blur
        CImg<uint> high = img;
        simple_gaussian_blur(high, sigma_high, boundary_conditions);

        if (sigma_low > 0.0f)
            simple_gaussian_blur(img, sigma_low, boundary_conditions);

        // 2. Precompute Blending LUT (Integer Math)
        // Max gradient is 255+255 = 510. Size 512 for safety.
        uint8_t alpha_lut[512];
        const float invT = (edge_thresh > 1e-6f) ? (1.0f / edge_thresh) : 0.0f;

        for (int i = 0; i < 512; ++i)
        {
            float grad = (float)i;
            // Calculate t = clamp(grad / threshold)
            float t = (invT > 0.0f) ? std::min(grad * invT, 1.0f) : 1.0f;

            // Smoothstep: a = t*t*(3 - 2*t)
            // a=1 means "High Gradient" -> Use Low Blur (preserve edge)
            // a=0 means "Low Gradient"  -> Use High Blur (flatten noise)
            float a = t * t * (3.0f - 2.0f * t);

            // Store as 0..255 integer
            alpha_lut[i] = static_cast<uint8_t>(a * 255.0f + 0.5f);
        }

        // 3. Parallel Blend
        // We read from 'img' (Low) and 'high' (High), calculate gradient on 'img',
        // and write result into 'high' (reusing memory).
#pragma omp parallel for collapse(3)
        for (int c = 0; c < s; ++c)
        {
            for (int z = 0; z < d; ++z)
            {
                for (int y = 0; y < h; ++y)
                {
                    // Pointers for fast access
                    // Low blur (Source for edges & color)
                    const uint* row_low = img.data(0, y, z, c);
                    const uint* row_low_prev = (y > 0) ? img.data(0, y - 1, z, c) : row_low;
                    const uint* row_low_next = (y < h - 1) ? img.data(0, y + 1, z, c) : row_low;

                    // High blur (Source for color & Destination for result)
                    uint* row_dest = high.data(0, y, z, c);

                    // 3a. Handle Left Edge (x=0)
                    {
                        int x = 0;
                        int val_l = row_low[x];
                        int val_r = (w > 1) ? row_low[x + 1] : val_l;

                        // Simple Gradient (Forward difference at edge)
                        int dx = std::abs(val_r - val_l);
                        int dy = std::abs((int)row_low_next[x] - (int)row_low_prev[x]);

                        int grad = std::min(dx + dy, 511);

                        uint alpha = alpha_lut[grad];
                        uint low_val = row_low[x];
                        uint high_val = row_dest[x]; // Current value in 'high' image

                        // Blend: (alpha * Low + (255-alpha) * High) / 255
                        // Fast approx: >> 8
                        row_dest[x] = (alpha * low_val + (255 - alpha) * high_val) >> 8;
                    }

                    // 3b. Fast Center Loop (No boundary checks)
                    // Uses pointers directly, SIMD-friendly
                    for (int x = 1; x < w - 1; ++x)
                    {
                        // Gradient on Low Blur image
                        // dx = abs(right - left), dy = abs(down - up)
                        int dx = std::abs((int)row_low[x + 1] - (int)row_low[x - 1]);
                        int dy = std::abs((int)row_low_next[x] - (int)row_low_prev[x]);

                        // Clamp to LUT size
                        int grad = std::min(dx + dy, 511);

                        uint alpha = alpha_lut[grad];
                        uint low_val = row_low[x];
                        uint high_val = row_dest[x];

                        // Integer Blend
                        row_dest[x] = (alpha * low_val + (255 - alpha) * high_val) >> 8;
                    }

                    // 3c. Handle Right Edge (x=w-1)
                    if (w > 1)
                    {
                        int x = w - 1;
                        int val_l = row_low[x - 1];
                        int val_r = row_low[x];

                        int dx = std::abs(val_r - val_l);
                        int dy = std::abs((int)row_low_next[x] - (int)row_low_prev[x]);

                        int grad = std::min(dx + dy, 511);

                        uint alpha = alpha_lut[grad];
                        uint low_val = row_low[x];
                        uint high_val = row_dest[x];

                        row_dest[x] = (alpha * low_val + (255 - alpha) * high_val) >> 8;
                    }
                }
            }
        }

        // 4. Output Swap
        // The result is currently in 'high'. We want it in 'img'.
        img.swap(high);
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

    void simple_median_blur(CImg<uint> &image, int kernel_size, float threshold) { image.blur_median(kernel_size, threshold); }


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

    // Fast 3x3 Median (Network Sort) - operates on local copies
    static inline uint fast_median_3x3(uint p0, uint p1, uint p2, uint p3, uint p4, uint p5, uint p6, uint p7, uint p8)
    {
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

    static inline void get_min_med_max_from_hist(const uint16_t* hist, int total, uint &zmin, uint &zmed, uint &zmax)
    {
        // Find min
        int i = 0;
        while (i < 256 && hist[i] == 0)
            ++i;
        zmin = i;

        // Find max
        int j = 255;
        while (j >= 0 && hist[j] == 0)
            --j;
        zmax = j;

        // Find median
        int target = (total + 1) / 2;
        int cum = 0;
        for (int k = i; k <= j; ++k)
        {
            cum += hist[k];
            if (cum >= target)
            {
                zmed = k;
                return;
            }
        }
        zmed = zmax;
    }

    // ===================== Adaptive Median Filter =====================

    void adaptive_median_filter(CImg<uint> &img, int max_window_size, int block_h)
    {
        if (img.is_empty())
            return;

        const int w = img.width();
        const int h = img.height();
        const int d = img.depth();
        const int s = img.spectrum();

        if (w < 2 || h < 2)
            return;

        // Validate window size
        if (max_window_size < 3)
            max_window_size = 3;
        if ((max_window_size & 1) == 0)
            ++max_window_size; // Ensure odd

        const int max_r = (max_window_size - 1) / 2;
        if (block_h < 8)
            block_h = 8;

#pragma omp parallel
        {
            // OPTIMIZATION: Allocate one (Per Thread)
            // These variables persist as the thread processes multiple y0 blocks
            std::vector<uint> buffer_vec;
            uint16_t hist[256];

#pragma omp for collapse(3) schedule(dynamic)
            for (int c = 0; c < s; ++c)
            {
                for (int z = 0; z < d; ++z)
                {
                    // Loop over blocks
                    for (int y0 = 0; y0 < h; y0 += block_h)
                    {
                        const int y1 = std::min(y0 + block_h, h);
                        const int halo_h = (y1 - y0) + 2 * max_r;

                        // 3. REUSE MEMORY
                        // resize() only re-allocates if capacity is insufficient.
                        // Since buffer_vec persists across iterations of this thread,
                        // it will likely only allocate once (on the first block).
                        if (buffer_vec.size() < (size_t)halo_h * w)
                        {
                            buffer_vec.resize((size_t)halo_h * w);
                        }

                        uint* src_buf = buffer_vec.data();

                        // Copy Block + Halo
                        uint* base_img = img.data(0, 0, z, c);
                        for (int yy = 0; yy < halo_h; ++yy)
                        {
                            const int img_y = utils::clampi(y0 + yy - max_r, 0, h - 1);
                            const uint* src_row_ptr = base_img + (size_t)img_y * w;
                            std::copy_n(src_row_ptr, w, src_buf + (size_t)yy * w);
                        }

                        // Process Pixels
                        for (int y = y0; y < y1; ++y)
                        {
                            const int by = (y - y0) + max_r;
                            uint* out_ptr = base_img + (size_t)y * w;

                            const uint* r_m1 = src_buf + (size_t)(by - 1) * w;
                            const uint* r_0 = src_buf + (size_t)(by)*w;
                            const uint* r_p1 = src_buf + (size_t)(by + 1) * w;

                            for (int x = 0; x < w; ++x)
                            {
                                // ... (Keep your existing pixel logic exactly as is) ...
                                // 1. Fast Path: 3x3 Window
                                const int xm1 = (x > 0) ? x - 1 : 0;
                                const int xp1 = (x < w - 1) ? x + 1 : w - 1;

                                uint p0 = r_m1[xm1], p1 = r_m1[x], p2 = r_m1[xp1];
                                uint p3 = r_0[xm1], p4 = r_0[x], p5 = r_0[xp1];
                                uint p6 = r_p1[xm1], p7 = r_p1[x], p8 = r_p1[xp1];

                                uint zmed = fast_median_3x3(p0, p1, p2, p3, p4, p5, p6, p7, p8);

                                // Fast Min/Max (Compiler vectorizes std::min/max with initializer lists)
                                uint zmin = std::min({p0, p1, p2, p3, p4, p5, p6, p7, p8});
                                uint zmax = std::max({p0, p1, p2, p3, p4, p5, p6, p7, p8});

                                uint zxy = r_0[x];

                                // Level A Check (Is median reliable?)
                                if (zmed > zmin && zmed < zmax)
                                {
                                    // Level B Check (Is current pixel an impulse?)
                                    out_ptr[x] = (zxy > zmin && zxy < zmax) ? zxy : zmed;
                                    continue;
                                }

                                // 2. Slow Path: Expansion (Histogram)
                                // We only enter here if 3x3 failed (very noisy area)

                                // Clear histogram (fast memset)
                                std::fill(std::begin(hist), std::end(hist), 0);
                                hist[p0]++;
                                hist[p1]++;
                                hist[p2]++;
                                hist[p3]++;
                                hist[p4]++;
                                hist[p5]++;
                                hist[p6]++;
                                hist[p7]++;
                                hist[p8]++;

                                int total = 9;
                                uint out_val = zmed; // Default fallback

                                // Expand radius r from 2 to max_r
                                for (int r = 2; r <= max_r; ++r)
                                {
                                    int xl = utils::clampi(x - r, 0, w - 1);
                                    int xr = utils::clampi(x + r, 0, w - 1);
                                    // Note: 'by' is offset in buffer, so no clamp needed for y
                                    int yt = by - r;
                                    int yb = by + r;

                                    // Top & Bottom rows of the ring
                                    const uint* row_t = src_buf + (size_t)yt * w;
                                    const uint* row_b = src_buf + (size_t)yb * w;

                                    for (int k = -r + 1; k <= r - 1; ++k)
                                    {
                                        int xk = utils::clampi(x + k, 0, w - 1);
                                        hist[row_t[xk]]++;
                                        hist[row_b[xk]]++;
                                    }

                                    // Left & Right columns of the ring (including corners)
                                    for (int k = -r; k <= r; ++k)
                                    {
                                        const uint* row = src_buf + (size_t)(by + k) * w;
                                        hist[row[xl]]++;
                                        hist[row[xr]]++;
                                    }

                                    total = (2 * r + 1) * (2 * r + 1);
                                    get_min_med_max_from_hist(hist, total, zmin, zmed, zmax);

                                    if (zmed > zmin && zmed < zmax)
                                    {
                                        out_val = (zxy > zmin && zxy < zmax) ? zxy : zmed;
                                        break;
                                    }
                                }

                                out_ptr[x] = out_val;
                            }
                        }
                    }
                }
            }
        }
    }
} // namespace ite::filters
