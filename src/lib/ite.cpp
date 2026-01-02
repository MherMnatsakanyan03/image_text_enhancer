#include "ite.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

const float WEIGHT_R = 0.299f;
const float WEIGHT_G = 0.587f;
const float WEIGHT_B = 0.114f;

/**
 * @brief (Internal) Computes the Summed-Area Table (Integral Image).
 */
static CImg<double> calculate_integral_image(const CImg<double> &src) {
    CImg<double> integral_img(src.width(), src.height(), src.depth(), src.spectrum(), 0);

    cimg_forZC(src, z, c) {
        // Process each 2D slice
        // First row
        for (int x = 0; x < src.width(); ++x) {
            // Add current pixel to the sum of previous pixels in the row
            integral_img(x, 0, z, c) = src(x, 0, z, c) + (x > 0 ? integral_img(x - 1, 0, z, c) : 0.0);
        }
        // Remaining rows
        for (int y = 1; y < src.height(); ++y) {
            double row_sum = 0.0;
            for (int x = 0; x < src.width(); ++x) {
                // Add current pixel to the sum of previous pixels in the row
                row_sum += src(x, y, z, c);
                integral_img(x, y, z, c) = row_sum + integral_img(x, y - 1, z, c);
            }
        }
    }

    return integral_img;
}

/**
 * @brief (Internal) Computes the sum of a region from an integral image.
 */
static double get_area_sum(const CImg<double> &integral_img,
                           int x1, int y1, int z, int c,
                           int x2, int y2) {
    // Get sum at all four corners of the rectangle, handling boundary conditions
    const double D = integral_img(x2, y2, z, c);
    const double B = (x1 > 0) ? integral_img(x1 - 1, y2, z, c) : 0.0;
    const double C = (y1 > 0) ? integral_img(x2, y1 - 1, z, c) : 0.0;
    const double A = (x1 > 0 && y1 > 0) ? integral_img(x1 - 1, y1 - 1, z, c) : 0.0;

    // Inclusion-Exclusion Principle
    return D - B - C + A;
}

/**
 * @brief (Internal) Converts an image to grayscale, in-place.
 * Uses the standard luminance formula.
 */
static void to_grayscale_inplace(CImg<uint> &input_image) {
    if (input_image.spectrum() == 1) {
        return; // Already grayscale
    }

    // Create a new image with the correct 1-channel dimensions
    CImg<uint> gray_image(input_image.width(), input_image.height(), input_image.depth(), 1);

#pragma omp parallel for collapse(2)
    for (int z = 0; z < input_image.depth(); ++z) {
        for (int y = 0; y < input_image.height(); ++y) {
            for (int x = 0; x < input_image.width(); ++x) {
                // Read all 3 channels
                uint r = input_image(x, y, z, 0);
                uint g = input_image(x, y, z, 1);
                uint b = input_image(x, y, z, 2);
                // Calculate and set the new pixel value
                gray_image(x, y, z, 0) = static_cast<uint>(std::round(WEIGHT_R * r + WEIGHT_G * g + WEIGHT_B * b));
            }
        }
    }

    // Assign the new 1-channel image to the input reference
    input_image = gray_image;
}

/**
 * @brief (Internal) Converts a grayscale image to a binary (black and white) image, in-place.
 * Uses Sauvola's method for adaptive thresholding.
 */
static void binarize_inplace(CImg<uint> &input_image, const int window_size = 15, const float k = 0.2f, const float delta = 0.0f) {
    if (input_image.spectrum() != 1) {
        throw std::runtime_error("Sauvola requires a grayscale image.");
    }

    CImg<double> img_double = input_image; // Convert CImg<uint> to CImg<double>

    CImg<double> integral_img = calculate_integral_image(img_double);
    CImg<double> integral_sq_img = calculate_integral_image(img_double.get_sqr()); // Integral of (pixel*pixel)

    CImg<uint> output_image(input_image.width(), input_image.height(), input_image.depth(), 1);
    const float R = 128.0f; // Max std. dev (for normalization)
    const int w_half = window_size / 2;

#pragma omp parallel for collapse(2)
    for (int z = 0; z < input_image.depth(); ++z) {
        for (int y = 0; y < input_image.height(); ++y) {
            for (int x = 0; x < input_image.width(); ++x) {
                // Define the local window (clamp to edges)
                const int x1 = std::max(0, x - w_half);
                const int y1 = std::max(0, y - w_half);
                const int x2 = std::min(input_image.width() - 1, x + w_half);
                const int y2 = std::min(input_image.height() - 1, y + w_half);

                const double N = (x2 - x1 + 1) * (y2 - y1 + 1); // Number of pixels in window

                // Get sum and sum of squares from integral images
                const double sum = get_area_sum(integral_img, x1, y1, z, 0, x2, y2);
                const double sum_sq = get_area_sum(integral_sq_img, x1, y1, z, 0, x2, y2);

                // Calculate local mean and std. deviation
                const double mean = sum / N;
                const double std_dev = std::sqrt(std::max(0.0, (sum_sq / N) - (mean * mean)));

                // Calculate Sauvola's threshold
                const double threshold = mean * (1.0 + k * ((std_dev / R) - 1.0)) - delta;

                // Apply threshold
                output_image(x, y, z) = (input_image(x, y, z) > threshold) * 255;
            }
        }
    }

    input_image = output_image;
}

/**
 * @brief (Internal) Applies Gaussian denoising to an image, in-place.
 * Uses CImg's built-in blur function with neumann boundary conditions and isotropic blur.
 */
static void gaussian_denoise_inplace(CImg<uint> &input_image, float sigma) {
    input_image.blur(sigma, 1, true);
}

/**
 * @brief (Internal) Applies dilation to a binary image, in-place.
 */
static void dilation_inplace(CImg<uint> &input_image, int kernel_size) {
    if (input_image.spectrum() != 1) {
        throw std::runtime_error("Dilation requires a single-channel image.");
    }

    if (kernel_size <= 1) {
        return;
    }

    CImg<uint> source = input_image;

    int r = kernel_size / 2;
    int w = input_image.width();
    int h = input_image.height();
    int d = input_image.depth();

#pragma omp parallel for collapse(2)
    for (int i_d = 0; i_d < d; ++i_d) {
        for (int i_h = 0; i_h < h; ++i_h) {
            for (int i_w = 0; i_w < w; ++i_w) {
                // If the pixel is already white, it stays white (dilation only adds white)
                if (source(i_w, i_h, i_d) == 255) {
                    continue;
                }

                // If pixel is black, check neighbors
                bool hit = false;

                // Scan neighborhood
                for (int k_h = -r; k_h <= r && !hit; ++k_h) {
                    for (int k_w = -r; k_w <= r && !hit; ++k_w) {
                        int n_w = i_w + k_w;
                        int n_h = i_h + k_h;

                        // Boundary check
                        if (n_w >= 0 && n_w < w && n_h >= 0 && n_h < h) {
                            if (source(n_w, n_h, i_d) == 255) {
                                hit = true;
                            }
                        }
                    }
                }

                if (hit) {
                    input_image(i_w, i_h, i_d) = 255;
                }
            }
        }
    }
}

/**
 * @brief (Internal) Applies erosion to a binary image, replacing the original image.
 */
static void erosion_inplace(CImg<uint> &input_image, int kernel_size) {
    if (input_image.spectrum() != 1) {
        throw std::runtime_error("Erosion requires a grayscale image.");
    }

    if (kernel_size <= 1)
        return;

    CImg<uint> source = input_image;

    int r = kernel_size / 2;
    int w = input_image.width();
    int h = input_image.height();
    int d = input_image.depth();

#pragma omp parallel for collapse(2)
    for (int i_d = 0; i_d < d; ++i_d) {
        for (int i_h = 0; i_h < h; ++i_h) {
            for (int i_w = 0; i_w < w; ++i_w) {
                // If the pixel is already black, it stays black (erosion only removes white)
                if (source(i_w, i_h, i_d) == 0) {
                    continue;
                }

                // If pixel is white, check neighbors
                bool hit = false;

                // Scan neighborhood
                for (int k_h = -r; k_h <= r && !hit; ++k_h) {
                    for (int k_w = -r; k_w <= r && !hit; ++k_w) {
                        int n_w = i_w + k_w;
                        int n_h = i_h + k_h;

                        // Boundary check
                        if (n_w >= 0 && n_w < w && n_h >= 0 && n_h < h) {
                            if (source(n_w, n_h, i_d) == 0) {
                                hit = true;
                            }
                        }
                    }
                }

                if (hit) {
                    input_image(i_w, i_h, i_d) = 0;
                }
            }
        }
    }
}

static unsigned char clamp_u8(uint v) {
    return (v > 255u) ? 255u : (unsigned char)v;
}

// Fast-ish border mean (assume background lives near borders).
static double border_mean_u8(const CImg<unsigned char>& g) {
    const int W = g.width(), H = g.height();
    if (W <= 0 || H <= 0) return 0.0;

    const int b = std::max(1, (int)std::floor(0.05 * std::min(W, H))); // 5% border
    const int step = 2; // subsample for speed

    uint64_t sum = 0;
    uint64_t cnt = 0;
    const unsigned char* p = g.data();

    auto add = [&](int x, int y) {
        sum += p[y * W + x];
        ++cnt;
    };

    // top + bottom
    for (int y = 0; y < b; y += step)
        for (int x = 0; x < W; x += step) add(x, y);
    for (int y = H - b; y < H; y += step)
        for (int x = 0; x < W; x += step) add(x, y);

    // left + right (skip corners already counted, but it's fine either way)
    for (int y = b; y < H - b; y += step) {
        for (int x = 0; x < b; x += step) add(x, y);
        for (int x = W - b; x < W; x += step) add(x, y);
    }

    return cnt ? (double)sum / (double)cnt : 0.0;
}

static int otsu_threshold_u8(const CImg<unsigned char>& g) {
    int hist[256] = {0};
    const int W = g.width(), H = g.height();
    const int N = W * H;
    if (N <= 0) return 128;

    const unsigned char* p = g.data();
    for (int i = 0; i < N; ++i) ++hist[p[i]];

    double sum_all = 0.0;
    for (int t = 0; t < 256; ++t) sum_all += (double)t * (double)hist[t];

    double sum_b = 0.0;
    int w_b = 0;
    int w_f = 0;

    double max_between = -1.0;
    int best_t = 128;

    for (int t = 0; t < 256; ++t) {
        w_b += hist[t];
        if (w_b == 0) continue;

        w_f = N - w_b;
        if (w_f == 0) break;

        sum_b += (double)t * (double)hist[t];

        const double m_b = sum_b / (double)w_b;
        const double m_f = (sum_all - sum_b) / (double)w_f;

        const double between = (double)w_b * (double)w_f * (m_b - m_f) * (m_b - m_f);
        if (between > max_between) {
            max_between = between;
            best_t = t;
        }
    }
    return best_t;
}

// Score: variance of horizontal projection profile (row sums) on a stable central ROI.
// Binary pixels are 0/1. Rotate with nearest-neighbor to preserve sparsity and speed.
static double score_angle_variance(
    const CImg<unsigned char>& bin,
    int roi_x0, int roi_y0, int roi_w, int roi_h,
    double angle_deg
) {
    // interpolation=0 (nearest), boundary=0 (Dirichlet 0). We ignore borders anyway via ROI.
    CImg<unsigned char> rot = bin.get_rotate(angle_deg, /*interpolation*/0, /*boundary*/0);

    const int W = rot.width();
    const unsigned char* p = rot.data();

    double sum = 0.0;
    double sum_sq = 0.0;

    for (int y = roi_y0; y < roi_y0 + roi_h; ++y) {
        const unsigned char* row = p + y * W + roi_x0;
        int row_sum = 0;
        // tight loop
        for (int x = 0; x < roi_w; ++x) row_sum += row[x];

        sum += (double)row_sum;
        sum_sq += (double)row_sum * (double)row_sum;
    }

    const double invH = 1.0 / (double)roi_h;
    const double mean = sum * invH;
    return sum_sq * invH - mean * mean;
}

static std::pair<double,double> search_best_angle(
    const CImg<unsigned char>& bin,
    int roi_x0, int roi_y0, int roi_w, int roi_h,
    double start_deg, double end_deg, double step_deg
) {
    if (step_deg <= 0.0) return {0.0, -1.0};
    if (end_deg < start_deg) std::swap(start_deg, end_deg);

    const int N = (int)std::floor((end_deg - start_deg) / step_deg) + 1;
    double best_angle = 0.0;
    double best_score = -1.0;

#pragma omp parallel
    {
        double local_best_a = 0.0;
        double local_best_s = -1.0;

#pragma omp for schedule(static) nowait
        for (int i = 0; i < N; ++i) {
            const double a = start_deg + (double)i * step_deg;
            const double s = score_angle_variance(bin, roi_x0, roi_y0, roi_w, roi_h, a);
            if (s > local_best_s) {
                local_best_s = s;
                local_best_a = a;
            }
        }

#pragma omp critical
        {
            if (local_best_s > best_score) {
                best_score = local_best_s;
                best_angle = local_best_a;
            }
        }
    }

    return {best_angle, best_score};
}

/**
 * @brief (Internal) Deskews the image using a robust Projection Profile method.
 * - true grayscale + Otsu
 * - polarity-safe (detect background from border)
 * - crop to content to boost SNR and speed
 * - padding-robust scoring (stable central ROI)
 * - coarse-to-fine angle search
 * - OpenMP reduction with per-thread best (minimal contention)
 * - final rotate uses Neumann boundary to avoid black corners
 */
static void deskew_inplace(CImg<uint>& input_image) {
    const int inW = input_image.width();
    const int inH = input_image.height();
    if (inW <= 1 || inH <= 1) return;

    // Downscale for speed: target ~600px on the LONG side (better than width-only).
    constexpr double target_long = 600.0;
    const int long_side = std::max(inW, inH);
    double scale = target_long / (double)long_side;
    if (scale > 1.0) scale = 1.0;

    int new_w = std::max(1, (int)std::lround(inW * scale));
    int new_h = std::max(1, (int)std::lround(inH * scale));

    // Keep 1 channel in the resized working copy (we'll do proper luminance ourselves if RGB).
    CImg<uint> small = input_image.get_resize(new_w, new_h, 1, input_image.spectrum());

    // Convert to true 8-bit grayscale luminance.
    CImg<unsigned char> gray(new_w, new_h, 1, 1);
    if (small.spectrum() >= 3) {
        for (int y = 0; y < new_h; ++y) {
            for (int x = 0; x < new_w; ++x) {
                const uint r = small(x, y, 0, 0);
                const uint g = small(x, y, 0, 1);
                const uint b = small(x, y, 0, 2);
                // Rec.709 luma
                const double yv = 0.2126 * (double)r + 0.7152 * (double)g + 0.0722 * (double)b;
                gray(x, y) = (unsigned char)std::clamp((int)std::lround(yv), 0, 255);
            }
        }
    } else {
        // Single-channel or other: take channel 0 and clamp.
        for (int y = 0; y < new_h; ++y)
            for (int x = 0; x < new_w; ++x)
                gray(x, y) = clamp_u8(small(x, y, 0, 0));
    }

    // Threshold with Otsu (robust global threshold).
    const int t = otsu_threshold_u8(gray);

    // Decide polarity from border: border brighter than threshold => white page background.
    const double bmean = border_mean_u8(gray);
    const bool background_is_bright = (bmean >= (double)t);

    // Build binary: text/foreground = 1, background = 0.
    // If background bright => text is darker (<= t). Else text is brighter (> t).
    CImg<unsigned char> bin(new_w, new_h, 1, 1);
    {
        const int N = new_w * new_h;
        const unsigned char* pg = gray.data();
        unsigned char* pb = bin.data();

        int fg = 0;
        if (background_is_bright) {
            for (int i = 0; i < N; ++i) {
                const unsigned char v = pg[i];
                const unsigned char b = (v <= (unsigned char)t) ? 1u : 0u;
                pb[i] = b;
                fg += b;
            }
        } else {
            for (int i = 0; i < N; ++i) {
                const unsigned char v = pg[i];
                const unsigned char b = (v > (unsigned char)t) ? 1u : 0u;
                pb[i] = b;
                fg += b;
            }
        }

        // Safety: if foreground becomes the majority (inverted case), flip it.
        // Helps when Otsu lands oddly on very clean pages.
        if (fg > N / 2) {
            for (int i = 0; i < N; ++i) pb[i] = (unsigned char)(1u - pb[i]);
        }
    }

    // Crop to content (tight bounding box around foreground) + small margin.
    int minx = new_w, miny = new_h, maxx = -1, maxy = -1;
    {
        const unsigned char* pb = bin.data();
        for (int y = 0; y < new_h; ++y) {
            const unsigned char* row = pb + y * new_w;
            for (int x = 0; x < new_w; ++x) {
                if (row[x]) {
                    minx = std::min(minx, x);
                    miny = std::min(miny, y);
                    maxx = std::max(maxx, x);
                    maxy = std::max(maxy, y);
                }
            }
        }
    }

    // If no content detected, do nothing.
    if (maxx < 0 || maxy < 0) return;

    const int margin = std::max(2, (int)std::lround(0.02 * std::min(new_w, new_h)));
    minx = std::max(0, minx - margin);
    miny = std::max(0, miny - margin);
    maxx = std::min(new_w - 1, maxx + margin);
    maxy = std::min(new_h - 1, maxy + margin);

    CImg<unsigned char> work = bin.get_crop(minx, miny, maxx, maxy);
    const int W = work.width();
    const int H = work.height();
    if (W <= 8 || H <= 8) return;

    // Stable central ROI to reduce rotation padding artifacts.
    const double pad = 0.10; // ignore 10% border on each side
    const int roi_w = std::max(1, (int)std::lround(W * (1.0 - 2.0 * pad)));
    const int roi_h = std::max(1, (int)std::lround(H * (1.0 - 2.0 * pad)));
    const int roi_x0 = (W - roi_w) / 2;
    const int roi_y0 = (H - roi_h) / 2;

    // Baseline score at 0° for "no-rotate" gate.
    const double base_score = score_angle_variance(work, roi_x0, roi_y0, roi_w, roi_h, 0.0);

    // Coarse-to-fine search within +/- 15 degrees.
    auto [a1, s1] = search_best_angle(work, roi_x0, roi_y0, roi_w, roi_h, -15.0, 15.0, 1.0);
    auto [a2, s2] = search_best_angle(work, roi_x0, roi_y0, roi_w, roi_h, a1 - 1.0, a1 + 1.0, 0.2);
    auto [a3, s3] = search_best_angle(work, roi_x0, roi_y0, roi_w, roi_h, a2 - 0.3, a2 + 0.3, 0.05);

    const double best_angle = a3;
    const double best_score = s3;

    // Rotate only if:
    //  - angle is non-trivial, AND
    //  - improvement over 0° is meaningful (avoid random rotations on low-text/noisy pages)
    const double abs_a = std::abs(best_angle);
    const bool angle_ok = (abs_a > 0.05);
    const bool improve_ok =
        (best_score > base_score + 1e-9) &&
        (base_score <= 0.0 ? true : (best_score >= base_score * 1.002)); // ~0.2% gain

    if (angle_ok && improve_ok) {
        // interpolation=2 (cubic) for quality.
        // boundary=1 (Neumann): repeat edge pixels to avoid black corners regardless of background.
        input_image.rotate(best_angle, /*interpolation*/2, /*boundary*/1);
    }
}

/**
 * @brief (Internal) Robust Linear Contrast Stretching.
 * Clips the bottom 1% and top 1% of intensities to ignore outliers,
 * then stretches the remaining range to 0-255.
 */
static void contrast_enhancement_inplace(CImg<uint> &input_image) {
    if (input_image.is_empty()) {
        return;
    }

    // Compute Histogram (to find percentiles)
    const CImg<uint> hist = input_image.get_histogram(256, 0, 255);
    const uint total_pixels = input_image.size();

    // Find lower (1%) and upper (99%) cutoffs
    const uint cutoff = total_pixels / 100; // 1% threshold

    uint min_val = 0;
    uint count = 0;
    // find the brightest pixel above cutoff
    for (int i = 0; i < 256; ++i) {
        count += hist[i];
        if (count > cutoff) {
            min_val = i;
            break;
        }
    }

    // find the darkest pixel below cutoff
    uint max_val = 255;
    count = 0;
    for (int i = 255; i >= 0; --i) {
        count += hist[i];
        if (count > cutoff) {
            max_val = i;
            break;
        }
    }

    // Safety check: if image is solid color, min might equal max
    if (max_val <= min_val) {
        return;
    }

    // Apply the stretch
    // Formula: 255 * (val - min) / (max - min)
    const float scale = 255.0f / (max_val - min_val);

#pragma omp parallel for
    for (uint i = 0; i < total_pixels; ++i) {
        uint val = input_image[i];

        if (val <= min_val) {
            input_image[i] = 0;
        } else if (val >= max_val) {
            input_image[i] = 255;
        } else {
            input_image[i] = (val - min_val) * scale;
        }
    }
}

/**
 * @brief (Internal) Removes small connected components (speckles) from the image.
 * Assumes the input is binary (0 and 255).
 * @param input_image The image to process.
 * @param threshold Components smaller than this number of pixels will be removed.
 * @param diagonal_connections
 */
static void despeckle_inplace(CImg<uint> &input_image, const uint threshold, bool diagonal_connections = true) {
    if (threshold <= 0)
        return;

    // Invert image so Text/Noise becomes White (255) and Background becomes Black (0).
    // CImg's label() function tracks non-zero regions.
#pragma omp parallel for
    for (uint i = 0; i < input_image.size(); ++i) {
        input_image[i] = (input_image[i] == 0) ? 255 : 0;
    }

    // Label connected components.
    // This assigns a unique integer ID (1, 2, 3...) to every distinct blob.
    // 0 is background.
    // true = 8-connectivity (diagonal pixels connect), false = 4-connectivity
    CImg<uint> labels = input_image.get_label(diagonal_connections);

    // Count the size of each component.
    uint max_label = labels.max();
    if (max_label == 0) {
        // Image was completely empty (or full), revert inversion and return
        input_image.fill(255);
        return;
    }

    std::vector<uint> sizes(max_label + 1, 0);

    // Iterate over the label image to count pixels per label
    // ptr points to a number in labels at each pixel which we
    // use as index in our sized vector
    cimg_for(labels, ptr, uint) {
        sizes[*ptr]++;
    }

    // Filter: If a label is too small, turn it off (Black -> 0) in our inverted map
#pragma omp parallel for collapse(2)
    for (int z = 0; z < input_image.depth(); ++z) {
        for (int y = 0; y < input_image.height(); ++y) {
            for (int x = 0; x < input_image.width(); ++x) {
                uint label_id = labels(x, y, z);
                // If it's a valid object (id > 0) AND it's smaller than threshold
                if (label_id > 0 && sizes[label_id] < threshold) {
                    input_image(x, y, z) = 0; // Erase it (in inverted space)
                }
            }
        }
    }

    // Invert back to original (Black Text, White Background)
#pragma omp parallel for
    for (uint i = 0; i < input_image.size(); ++i) {
        input_image[i] = (input_image[i] == 255) ? 0 : 255;
    }
}

static int clampi(int v, int lo, int hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static uint clamp_u8_from_float(float v) {
    int iv = (int) std::lround(v);
    if (iv < 0) return 0u;
    if (iv > 255) return 255u;
    return (uint) iv;
}

/**
 * Fast separable Gaussian blur (Neumann/replicate boundary), in-place.
 * Extra memory: O(max(w,h)) per thread (row/col scratch).
 */
static void gaussian_blur_inplace_omp(CImg<uint> &img, float sigma, int truncate = 3) {
    if (img.is_empty() || sigma <= 0.0f) return;

    const int w = img.width();
    const int h = img.height();
    const int d = img.depth();
    const int s = img.spectrum();

    if (w <= 1 && h <= 1) return;

    const int r = std::max(1, (int) std::ceil(truncate * sigma));
    const int ksz = 2 * r + 1;

    std::vector<float> kernel(ksz);
    {
        const float inv2s2 = 1.0f / (2.0f * sigma * sigma);
        float sum = 0.0f;
        for (int i = -r; i <= r; ++i) {
            float v = std::exp(-(float) (i * i) * inv2s2);
            kernel[i + r] = v;
            sum += v;
        }
        const float invsum = 1.0f / sum;
        for (float &v: kernel) v *= invsum;
    }

    // --- Horizontal pass (write back in-place; safe with per-row scratch)
#pragma omp parallel
    {
        std::vector<float> tmp((size_t) w);

#pragma omp for collapse(3) schedule(static)
        for (int c = 0; c < s; ++c) {
            for (int z = 0; z < d; ++z) {
                for (int y = 0; y < h; ++y) {
                    uint *base = img.data(0, 0, z, c);
                    uint *row = base + (size_t) y * w;

                    // Left edge
                    for (int x = 0; x < std::min(r, w); ++x) {
                        float acc = 0.0f;
                        for (int k = -r; k <= r; ++k) {
                            const int xx = clampi(x + k, 0, w - 1);
                            acc += kernel[k + r] * (float) row[xx];
                        }
                        tmp[x] = acc;
                    }

                    // Center (no clamps)
                    for (int x = r; x < w - r; ++x) {
                        float acc = 0.0f;
#pragma omp simd reduction(+:acc)
                        for (int k = -r; k <= r; ++k) {
                            acc += kernel[k + r] * (float) row[x + k];
                        }
                        tmp[x] = acc;
                    }

                    // Right edge
                    for (int x = std::max(w - r, 0); x < w; ++x) {
                        float acc = 0.0f;
                        for (int k = -r; k <= r; ++k) {
                            const int xx = clampi(x + k, 0, w - 1);
                            acc += kernel[k + r] * (float) row[xx];
                        }
                        tmp[x] = acc;
                    }

                    // Write back
                    for (int x = 0; x < w; ++x) row[x] = clamp_u8_from_float(tmp[x]);
                }
            }
        }
    }

    // --- Vertical pass (must buffer full column results before writing to avoid read-after-write)
#pragma omp parallel
    {
        std::vector<float> tmp((size_t) h);

#pragma omp for collapse(3) schedule(static)
        for (int c = 0; c < s; ++c) {
            for (int z = 0; z < d; ++z) {
                for (int x = 0; x < w; ++x) {
                    uint *base = img.data(0, 0, z, c);

                    // Top edge
                    for (int y = 0; y < std::min(r, h); ++y) {
                        float acc = 0.0f;
                        for (int k = -r; k <= r; ++k) {
                            const int yy = clampi(y + k, 0, h - 1);
                            acc += kernel[k + r] * (float) base[(size_t) yy * w + x];
                        }
                        tmp[y] = acc;
                    }

                    // Center (no clamps)
                    for (int y = r; y < h - r; ++y) {
                        float acc = 0.0f;
#pragma omp simd reduction(+:acc)
                        for (int k = -r; k <= r; ++k) {
                            acc += kernel[k + r] * (float) base[(size_t) (y + k) * w + x];
                        }
                        tmp[y] = acc;
                    }

                    // Bottom edge
                    for (int y = std::max(h - r, 0); y < h; ++y) {
                        float acc = 0.0f;
                        for (int k = -r; k <= r; ++k) {
                            const int yy = clampi(y + k, 0, h - 1);
                            acc += kernel[k + r] * (float) base[(size_t) yy * w + x];
                        }
                        tmp[y] = acc;
                    }

                    // Write back
                    for (int y = 0; y < h; ++y) base[(size_t) y * w + x] = clamp_u8_from_float(tmp[y]);
                }
            }
        }
    }
}

// ================= Adaptive Gaussian blur (edge-adaptive blend of two Gaussians) =================
// Idea: compute a low-sigma blur (preserve edges) and a high-sigma blur (smooth flats),
// then blend per-pixel using an edge strength measure (fast gradient).
// Space/time efficient: 1 extra full image (high blur) + per-thread row-block buffer; 2 separable blurs + 1 blend pass.
// Parallel: OpenMP over (channel, depth, row-block).

// In-place adaptive Gaussian blur
static void adaptive_gaussian_blur_inplace_omp(
    CImg<uint> &img,
    float sigma_low,
    float sigma_high,
    float edge_thresh, // gradient threshold controlling blend (typical 30..80 for 8-bit)
    int truncate = 3,
    int block_h = 64
) {
    if (img.is_empty()) return;

    const int w = img.width();
    const int h = img.height();
    const int d = img.depth();
    const int s = img.spectrum();

    if (w <= 1 || h <= 1) {
        // degenerate: just do a normal blur (or nothing)
        if (sigma_low > 0.0f) gaussian_blur_inplace_omp(img, sigma_low, truncate);
        return;
    }

    // If no real adaptation requested, fall back to regular blur
    if (!(sigma_high > sigma_low) || sigma_high <= 0.0f) {
        if (sigma_low > 0.0f) gaussian_blur_inplace_omp(img, sigma_low, truncate);
        return;
    }

    if (block_h < 8) block_h = 8;

    // 1) Compute the high-sigma blur into a single extra image
    CImg<uint> high = img;
    gaussian_blur_inplace_omp(high, sigma_high, truncate);

    // 2) Compute the low-sigma blur in-place (img becomes "low")
    if (sigma_low > 0.0f) gaussian_blur_inplace_omp(img, sigma_low, truncate);

    // 3) Blend using edge strength from the low-blur image (row-block buffering => safe in-place write)
    const float invT = (edge_thresh > 1e-6f) ? (1.0f / edge_thresh) : 0.0f;

#pragma omp parallel for collapse(3) schedule(static)
    for (int c = 0; c < s; ++c) {
        for (int z = 0; z < d; ++z) {
            for (int y0 = 0; y0 < h; y0 += block_h) {
                const int y1 = std::min(y0 + block_h, h);
                const int halo_h = (y1 - y0) + 2; // +2 for y-1 and y+1 (r=1)
                std::vector<uint> lowbuf((size_t) halo_h * w);

                uint *low_base = img.data(0, 0, z, c);
                const uint *hi_base = high.data(0, 0, z, c);

                // Copy low-blur block + halo rows into thread-local buffer (replicate boundary)
                for (int yy = 0; yy < halo_h; ++yy) {
                    const int ys = clampi(y0 + yy - 1, 0, h - 1);
                    const uint *src = low_base + (size_t) ys * w;
                    uint *dst = lowbuf.data() + (size_t) yy * w;
                    std::copy(src, src + w, dst);
                }

                // Blend and write back into img
                for (int y = y0; y < y1; ++y) {
                    const int by = (y - y0) + 1; // offset into lowbuf (accounts for halo)
                    const uint *r_up = lowbuf.data() + (size_t) (by - 1) * w;
                    const uint *r_mid = lowbuf.data() + (size_t) (by) * w;
                    const uint *r_down = lowbuf.data() + (size_t) (by + 1) * w;

                    uint *out = low_base + (size_t) y * w;
                    const uint *hi = hi_base + (size_t) y * w;

                    // x = 0 (replicate left)
                    {
                        const int dx = (int) r_mid[1] - (int) r_mid[0];
                        const int dy = (int) r_down[0] - (int) r_up[0];
                        float grad = (float) (std::abs(dx) + std::abs(dy)); // fast L1 magnitude

                        float t = invT > 0.0f ? clampf(grad * invT, 0.0f, 1.0f) : 1.0f;
                        // smoothstep for stable blending
                        float a = t * t * (3.0f - 2.0f * t); // a=1 -> prefer low (edges), a=0 -> prefer high (flats)

                        const float lv = (float) r_mid[0];
                        const float hv = (float) hi[0];
                        out[0] = clamp_u8_from_float(a * lv + (1.0f - a) * hv);
                    }

                    // center
                    for (int x = 1; x < w - 1; ++x) {
                        const int dx = (int) r_mid[x + 1] - (int) r_mid[x - 1];
                        const int dy = (int) r_down[x] - (int) r_up[x];
                        float grad = (float) (std::abs(dx) + std::abs(dy));

                        float t = invT > 0.0f ? clampf(grad * invT, 0.0f, 1.0f) : 1.0f;
                        float a = t * t * (3.0f - 2.0f * t);

                        const float lv = (float) r_mid[x];
                        const float hv = (float) hi[x];
                        out[x] = clamp_u8_from_float(a * lv + (1.0f - a) * hv);
                    }

                    // x = w-1 (replicate right)
                    {
                        const int xm1 = w - 2;
                        const int x = w - 1;
                        const int dx = (int) r_mid[x] - (int) r_mid[xm1];
                        const int dy = (int) r_down[x] - (int) r_up[x];
                        float grad = (float) (std::abs(dx) + std::abs(dy));

                        float t = invT > 0.0f ? clampf(grad * invT, 0.0f, 1.0f) : 1.0f;
                        float a = t * t * (3.0f - 2.0f * t);

                        const float lv = (float) r_mid[x];
                        const float hv = (float) hi[x];
                        out[x] = clamp_u8_from_float(a * lv + (1.0f - a) * hv);
                    }
                }
            }
        }
    }
}

// --------- Median-of-9 (3x3) fast network ----------
static inline void pix_sort(uint &a, uint &b) {
    if (a > b) {
        uint t = a;
        a = b;
        b = t;
    }
}

static inline uint median9(uint p0, uint p1, uint p2,
                           uint p3, uint p4, uint p5,
                           uint p6, uint p7, uint p8) {
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
static inline void hist_add(std::array<uint16_t,256>& hist,
                            std::array<uint8_t,256>& touched,
                            int& ntouched,
                            uint v)
{
    const uint8_t b = (uint8_t)v;
    if (hist[b]++ == 0) touched[ntouched++] = b;
}

static inline void hist_reset(std::array<uint16_t,256>& hist,
                              const std::array<uint8_t,256>& touched,
                              int ntouched)
{
    for (int i = 0; i < ntouched; ++i) hist[touched[i]] = 0;
}

static inline void hist_min_med_max(const std::array<uint16_t,256>& hist,
                                    int total,
                                    uint& zmin, uint& zmed, uint& zmax)
{
    // min
    int i = 0;
    while (i < 256 && hist[i] == 0) ++i;
    zmin = (i < 256) ? (uint)i : 0u;

    // max
    int j = 255;
    while (j >= 0 && hist[j] == 0) --j;
    zmax = (j >= 0) ? (uint)j : 255u;

    // median
    const int target = (total + 1) / 2;
    int cum = 0;
    for (int k = 0; k < 256; ++k) {
        cum += (int)hist[k];
        if (cum >= target) { zmed = (uint)k; return; }
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
static void adaptive_median_filter_inplace_omp(CImg<uint>& img, int max_window_size = 7, int block_h = 64)
{
    if (img.is_empty()) return;

    const int w = img.width();
    const int h = img.height();
    const int d = img.depth();
    const int s = img.spectrum();
    if (w < 2 || h < 2) return;

    if (max_window_size < 3) max_window_size = 3;
    if ((max_window_size & 1) == 0) ++max_window_size;
    const int max_r = (max_window_size - 1) / 2;

    if (block_h < 8) block_h = 8;

#pragma omp parallel for collapse(3) schedule(static)
    for (int c = 0; c < s; ++c) {
        for (int z = 0; z < d; ++z) {
            for (int y0 = 0; y0 < h; y0 += block_h) {

                const int y1 = std::min(y0 + block_h, h);
                const int halo_h = (y1 - y0) + 2 * max_r;

                // Copy block + halo rows into thread-local buffer (replicate boundary).
                std::vector<uint> src((size_t)halo_h * w);
                uint* base = img.data(0, 0, z, c);
                for (int yy = 0; yy < halo_h; ++yy) {
                    const int ys = clampi(y0 + yy - max_r, 0, h - 1);
                    const uint* row_src = base + (size_t)ys * w;
                    uint* row_dst = src.data() + (size_t)yy * w;
                    std::copy(row_src, row_src + w, row_dst);
                }

                // Per-thread reusable histogram state (no per-pixel allocations).
                std::array<uint16_t,256> hist{};
                std::array<uint8_t,256>  touched{};

                // Process rows in block; write back into original image.
                for (int y = y0; y < y1; ++y) {
                    const int by = (y - y0) + max_r; // center row index in src buffer
                    uint* out = base + (size_t)y * w;

                    const uint* r_m1 = src.data() + (size_t)(by - 1) * w;
                    const uint* r_0  = src.data() + (size_t)(by    ) * w;
                    const uint* r_p1 = src.data() + (size_t)(by + 1) * w;

                    for (int x = 0; x < w; ++x) {
                        const int xm1 = (x == 0) ? 0 : x - 1;
                        const int xp1 = (x == w - 1) ? (w - 1) : x + 1;

                        const uint zxy = r_0[x];

                        // Stage with 3x3 using very fast ops
                        uint p0=r_m1[xm1], p1=r_m1[x],  p2=r_m1[xp1];
                        uint p3=r_0 [xm1], p4=r_0 [x],  p5=r_0 [xp1];
                        uint p6=r_p1[xm1], p7=r_p1[x],  p8=r_p1[xp1];

                        uint zmed = median9(p0,p1,p2,p3,p4,p5,p6,p7,p8);
                        uint zmin = p0, zmax = p0;
                        // min/max of 9 (cheap)
                        uint arr9[9] = {p0,p1,p2,p3,p4,p5,p6,p7,p8};
                        for (int i = 1; i < 9; ++i) { zmin = std::min(zmin, arr9[i]); zmax = std::max(zmax, arr9[i]); }

                        // AMF Stage A / B decision at r=1
                        if (zmed > zmin && zmed < zmax) {
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
                        for (int r = 2; r <= max_r; ++r) {
                            // Add ring pixels (8r pixels) with replicate boundary in x, halo already in y.
                            const int xl = clampi(x - r, 0, w - 1);
                            const int xr = clampi(x + r, 0, w - 1);

                            // Vertical sides for dy in [-r..r]
                            for (int dy = -r; dy <= r; ++dy) {
                                const uint* row = src.data() + (size_t)(by + dy) * w;
                                hist_add(hist, touched, ntouched, row[xl]);
                                hist_add(hist, touched, ntouched, row[xr]);
                            }
                            // Top & bottom (excluding corners) for dx in [-(r-1)..(r-1)]
                            const uint* rowt = src.data() + (size_t)(by - r) * w;
                            const uint* rowb = src.data() + (size_t)(by + r) * w;
                            for (int dx = -(r - 1); dx <= (r - 1); ++dx) {
                                const int xx = clampi(x + dx, 0, w - 1);
                                hist_add(hist, touched, ntouched, rowt[xx]);
                                hist_add(hist, touched, ntouched, rowb[xx]);
                            }

                            total = (2 * r + 1) * (2 * r + 1);
                            hist_min_med_max(hist, total, zmin, zmed, zmax);

                            if (zmed > zmin && zmed < zmax) {
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

// ===================== Noise / edge estimators (parallel + histogram based) =====================

static float estimate_noise_sigma_mad_diffs_omp(const CImg<uint>& gray, int step = 2)
{
    // Robust sigma estimate from median absolute differences (horizontal+vertical).
    // For Gaussian noise: median(|diff|) ≈ 0.6745 * sigma * sqrt(2)
    const int w = gray.width(), h = gray.height();
    if (w < 2 || h < 2) return 0.0f;
    if (step < 1) step = 1;

    std::array<uint64_t,256> hist{}; // global

#pragma omp parallel
    {
        std::array<uint64_t,256> local{};
#pragma omp for schedule(static)
        for (int y = 0; y < h; y += step) {
            const uint* row = gray.data(0, y, 0, 0);
            for (int x = 0; x < w - 1; x += step) {
                uint a = row[x], b = row[x + 1];
                uint d = (a > b) ? (a - b) : (b - a);
                local[(uint8_t)d]++;
            }
        }
#pragma omp for schedule(static)
        for (int y = 0; y < h - 1; y += step) {
            const uint* row  = gray.data(0, y, 0, 0);
            const uint* row2 = gray.data(0, y + 1, 0, 0);
            for (int x = 0; x < w; x += step) {
                uint a = row[x], b = row2[x];
                uint d = (a > b) ? (a - b) : (b - a);
                local[(uint8_t)d]++;
            }
        }
#pragma omp critical
        {
            for (int i = 0; i < 256; ++i) hist[i] += local[i];
        }
    }

    uint64_t total = 0;
    for (int i = 0; i < 256; ++i) total += hist[i];
    if (total == 0) return 0.0f;

    const uint64_t target = (total + 1) / 2;
    uint64_t cum = 0;
    int med = 0;
    for (; med < 256; ++med) {
        cum += hist[med];
        if (cum >= target) break;
    }

    const float denom = 0.6745f * std::sqrt(2.0f);
    return (denom > 0.0f) ? ((float)med / denom) : 0.0f;
}

static float estimate_gradient_percentile_omp(const CImg<uint>& gray, float pct = 0.75f, int step = 2)
{
    // Gradient magnitude approx = |dx| + |dy| in [0..510]. Histogram percentile.
    const int w = gray.width(), h = gray.height();
    if (w < 2 || h < 2) return 0.0f;
    if (step < 1) step = 1;
    pct = clampf(pct, 0.0f, 1.0f);

    constexpr int GMAX = 510;
    std::array<uint64_t, GMAX + 1> hist{};

#pragma omp parallel
    {
        std::array<uint64_t, GMAX + 1> local{};
#pragma omp for schedule(static)
        for (int y = 0; y < h - 1; y += step) {
            const uint* row  = gray.data(0, y, 0, 0);
            const uint* row2 = gray.data(0, y + 1, 0, 0);
            for (int x = 0; x < w - 1; x += step) {
                uint a = row[x];
                uint dx = (a > row[x + 1]) ? (a - row[x + 1]) : (row[x + 1] - a);
                uint dy = (a > row2[x])    ? (a - row2[x])    : (row2[x] - a);
                uint g = dx + dy;
                if (g > (uint)GMAX) g = (uint)GMAX;
                local[g]++;
            }
        }
#pragma omp critical
        {
            for (int i = 0; i <= GMAX; ++i) hist[i] += local[i];
        }
    }

    uint64_t total = 0;
    for (int i = 0; i <= GMAX; ++i) total += hist[i];
    if (total == 0) return 0.0f;

    const uint64_t target = (uint64_t)std::ceil(pct * (double)total);
    uint64_t cum = 0;
    int val = 0;
    for (; val <= GMAX; ++val) {
        cum += hist[val];
        if (cum >= target) break;
    }
    return (float)val;
}

// ===================== Choose sigmas (good defaults for text enhancement) =====================
// These are heuristics designed to:
//   - keep sigma_low small to preserve stroke edges,
//   - increase sigma_high with noise to smooth background/texture,
//   - set edge_thresh from gradient distribution so edges stay sharp.

struct AdaptiveGaussianParams {
    float sigma_low;
    float sigma_high;
    float edge_thresh;
};

static AdaptiveGaussianParams choose_sigmas_for_text_enhancement(const CImg<uint>& gray)
{
    // Require grayscale 1-channel image.
    const float noise = estimate_noise_sigma_mad_diffs_omp(gray, 2);
    const float g75   = estimate_gradient_percentile_omp(gray, 0.75f, 2);
    const float g90   = estimate_gradient_percentile_omp(gray, 0.90f, 2);

    // Base sigmas from noise (bounded to stay text-friendly)
    float sigma_low  = clampf(0.45f + 0.030f * noise, 0.50f, 1.25f);
    float sigma_high = clampf(1.10f + 0.060f * noise, 1.10f, 2.80f);

    // If image is already blurry (weak strong-grad), be more conservative
    if (g90 < 70.0f) {
        sigma_low  *= 0.85f;
        sigma_high *= 0.85f;
    }

    // Edge threshold for adaptive blend: tie to gradient distribution.
    // Higher => more pixels treated as "flat" (more high-sigma smoothing).
    float edge_thresh = clampf(std::max(25.0f, 0.90f * g75), 25.0f, 160.0f);

    return { sigma_low, sigma_high, edge_thresh };
}

static int choose_adaptive_median_max_window(const CImg<uint>& gray)
{
    // Pick max window size based on estimated noise.
    const float noise = estimate_noise_sigma_mad_diffs_omp(gray, 2);
    if (noise < 4.0f)  return 3;
    if (noise < 8.0f)  return 5;
    if (noise < 14.0f) return 7;
    return 9;
}

namespace ite {
    CImg<uint> loadimage(std::string filepath) {
        CImg<uint> image(filepath.c_str());
        return image;
    }

    void writeimage(const CImg<uint> &image, std::string filepath) {
        image.save(filepath.c_str());
    }

    CImg<uint> to_grayscale(const CImg<uint> &input_image) {
        CImg<uint> output_image = input_image;
        to_grayscale_inplace(output_image);
        return output_image;
    }

    CImg<uint> binarize(const CImg<uint> &input_image) {
        CImg<uint> output_image = input_image;

        if (output_image.spectrum() != 1)
            to_grayscale_inplace(output_image);

        binarize_inplace(output_image);
        return output_image;
    }

    CImg<uint> gaussian_blur(const CImg<uint> &input_image, float sigma, int truncate) {
        CImg<uint> output_image = input_image;
        gaussian_blur_inplace_omp(output_image, sigma, truncate);
        return output_image;
    }

    CImg<uint> gaussian_blur_std(const CImg<uint> &input_image, float sigma) {
        CImg<uint> output_image = input_image;
        gaussian_denoise_inplace(output_image, sigma);
        return output_image;
    }

    CImg<uint> adaptive_median_filter(const CImg<uint> &input_image, int block_h) {
        CImg<uint> output_image = input_image;
        adaptive_median_filter_inplace_omp(output_image, block_h);
        return output_image;
    }

    CImg<uint> gaussian_denoise(const CImg<uint> &input_image, float sigma) {
        CImg<uint> output_image = input_image;
        gaussian_blur_inplace_omp(output_image, sigma, /*truncate=*/3);
        return output_image;
    }

    CImg<uint> adaptive_gaussian_blur(
        const CImg<uint> &input_image,
        float sigma_low,
        float sigma_high,
        float edge_thresh,
        int truncate,
        int block_h
    ) {
        CImg<uint> output_image = input_image;
        adaptive_gaussian_blur_inplace_omp(output_image, sigma_low, sigma_high, edge_thresh, truncate, block_h);
        return output_image;
    }

    CImg<uint> dilation(const CImg<uint> &input_image, int kernel_size) {
        CImg<uint> output_image = input_image;
        dilation_inplace(output_image, kernel_size);
        return output_image;
    }

    CImg<uint> erosion(const CImg<uint> &input_image, int kernel_size) {
        CImg<uint> output_image = input_image;
        erosion_inplace(output_image, kernel_size);
        return output_image;
    }

    CImg<uint> deskew(const CImg<uint> &input_image) {
        CImg<uint> output_image = input_image;
        deskew_inplace(output_image);
        return output_image;
    }

    CImg<uint> contrast_enhancement(const CImg<uint> &input_image) {
        CImg<uint> output_image = input_image;
        contrast_enhancement_inplace(output_image);
        return output_image;
    }

    CImg<uint> despeckle(const CImg<uint> &input_image, uint threshold, bool diagonal_connections) {
        CImg<uint> output_image = input_image;
        despeckle_inplace(output_image, threshold, diagonal_connections);
        return output_image;
    }

    CImg<uint> median_filter_adaptive(const CImg<uint>& input_image, int max_window_size, int block_h)
    {
        CImg<uint> out = input_image;
        adaptive_median_filter_inplace_omp(out, max_window_size, block_h);
        return out;
    }

    // Auto-choose max window size for adaptive median
    int choose_median_max_window_for_text(const CImg<uint>& gray_image)
    {
        return choose_adaptive_median_max_window(gray_image);
    }

    // Auto-choose sigmas/thresholds for adaptive Gaussian blur
    AdaptiveGaussianParams choose_sigmas_for_text(const CImg<uint>& gray_image)
    {
        return choose_sigmas_for_text_enhancement(gray_image);
    }

    CImg<uint> enhance(
        const CImg<uint> &input_image,
        float sigma,
        int kernel_size,
        int despeckle_threshold,
        bool diagonal_connections,
        bool do_erosion,
        bool do_dilation,
        bool do_despeckle,
        bool do_deskew)
    {
        CImg<uint> l_image = input_image;

        // Preparation & Alignment
        to_grayscale_inplace(l_image);

        if (do_deskew) {
            deskew_inplace(l_image);
        }

        contrast_enhancement_inplace(l_image);

        // ----------------- Adaptive denoise (text-friendly) -----------------

        // 1) Adaptive median (impulse/speckle cleanup while preserving strokes)
        /*const int max_win = choose_adaptive_median_max_window(l_image); // 3/5/7/9 heuristic
        adaptive_median_filter_inplace_omp(l_image, max_win, 64);*/

        // 2) (Classic Gaussian blur)
        // gaussian_blur_inplace_omp(l_image, sigma, 3);
        (void)sigma;

        // 3) Adaptive Gaussian blur (edge-adaptive blend of low/high sigma)
        /*const AdaptiveGaussianParams p = choose_sigmas_for_text_enhancement(l_image);
        adaptive_gaussian_blur_inplace_omp(
            l_image,
            p.sigma_low,
            p.sigma_high,
            p.edge_thresh,
            3,
            64
        );*/

        // Binarize
        binarize_inplace(l_image);

        // Remove noise (little dust specks)
        if (do_despeckle) {
            despeckle_inplace(l_image, static_cast<uint>(despeckle_threshold), diagonal_connections);
        }

        // Shape Repair (Morphologicals)
        if (do_dilation) {
            dilation_inplace(l_image, kernel_size);
        }

        if (do_erosion) {
            erosion_inplace(l_image, kernel_size);
        }

        return l_image;
    }
}
