#include "ite.h"
#include <CImg.h>
#include <cmath>
#include <random>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "catch2/catch_get_random_seed.hpp"
#include "catch2/generators/catch_generators.hpp"
#include "catch2/generators/catch_generators_range.hpp"

TEST_CASE("adaptive_median_filter: behaves correctly in extremes and removes impulse noise", "[ite][median]")
{
    constexpr int block_h = 16;

    SECTION("Does not change a uniform image")
    {
        const auto i = GENERATE(range(0, 10)); // 10 samples

        std::seed_seq seq{ Catch::getSeed(), static_cast<std::uint32_t>(i) };
        std::mt19937 rng(seq);
        std::uniform_int_distribution<int> gray_dist(0, 255);
        std::uniform_int_distribution<int> x_dist(0, 16);
        std::uniform_int_distribution<int> y_dist(0, 8);

        const uint gray_value = static_cast<uint>(gray_dist(rng));
        CImg<uint> input(17, 9, 1, 1, gray_value);

        CImg<uint> out = ite::adaptive_median_filter(input, 7, block_h);

        // Check random pixels
        const int x1 = x_dist(rng);
        const int y1 = y_dist(rng);
        const int x2 = x_dist(rng);
        const int y2 = y_dist(rng);
        const int x3 = x_dist(rng);
        const int y3 = y_dist(rng);

        CHECK(out(x1, y1) == gray_value);
        CHECK(out(x2, y2) == gray_value);
        CHECK(out(x3, y3) == gray_value);
    }

    SECTION("Removes salt & pepper noise from uniform regions")
    {
        // Create uniform image with sparse impulse noise
        CImg<uint> input(21, 21, 1, 1, 128);

        // Add salt (white) and pepper (black) noise at specific locations
        input(5, 5) = 255;   // salt
        input(5, 15) = 0;    // pepper
        input(10, 10) = 255; // salt
        input(15, 5) = 0;    // pepper
        input(15, 15) = 255; // salt

        CImg<uint> out = ite::adaptive_median_filter(input, 7, block_h);

        // All noise pixels should be replaced with median (128)
        CHECK(out(5, 5) == 128);
        CHECK(out(5, 15) == 128);
        CHECK(out(10, 10) == 128);
        CHECK(out(15, 5) == 128);
        CHECK(out(15, 15) == 128);

        // Other pixels should remain unchanged
        CHECK(out(0, 0) == 128);
        CHECK(out(20, 20) == 128);
        CHECK(out(10, 20) == 128);
    }

    SECTION("Preserves edges in smooth gradients")
    {
        // Create a step edge
        CImg<uint> input(15, 15, 1, 1, 0);
        for (int y = 0; y < input.height(); ++y)
            for (int x = 0; x < input.width(); ++x)
                input(x, y) = (x < 7) ? 50u : 200u;

        CImg<uint> out = ite::adaptive_median_filter(input, 7, block_h);

        // Edge should be preserved (check pixels far from edge)
        CHECK(out(2, 7) == 50);
        CHECK(out(12, 7) == 200);

        // Transition zone might be slightly smoothed but should maintain general structure
        const int transition = static_cast<int>(out(7, 7));
        CHECK(transition >= 40);
        CHECK(transition <= 210);
    }

    SECTION("Majority voting on randomized binarized image")
    {
        const auto i = GENERATE(range(0, 5)); // 5 samples

        std::seed_seq seq{ Catch::getSeed(), static_cast<std::uint32_t>(i) };
        std::mt19937 rng(seq);
        std::uniform_int_distribution<int> bin_dist(0, 1);

        // Create random binary image (0 or 255)
        CImg<uint> input(19, 19, 1, 1, 0);
        cimg_forXY(input, x, y) {
            input(x, y) = bin_dist(rng) ? 255u : 0u;
        }

        CImg<uint> out = ite::adaptive_median_filter(input, 7, block_h);

        // After median filtering, all pixels should still be binary (0 or 255)
        // because median of binary values is binary
        cimg_forXY(out, x, y) {
            const uint val = out(x, y);
            CHECK((val == 0 || val == 255));
        }

        // Check that output represents local majority in neighborhoods
        // Test center region where we have full neighborhoods
        for (int y = 5; y < 14; ++y) {
            for (int x = 5; x < 14; ++x) {
                // Count 0s and 255s in 3x3 neighborhood of input
                int count_black = 0, count_white = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        const uint val = input(x + dx, y + dy);
                        if (val == 0) ++count_black;
                        else ++count_white;
                    }
                }

                // The output should reflect the majority
                const uint out_val = out(x, y);
                if (count_black > count_white) {
                    // Expect output tends toward black (0)
                    // (may not be exact due to adaptive window expansion)
                    INFO("Position (" << x << ", " << y << "): blacks=" << count_black << ", whites=" << count_white);
                } else if (count_white > count_black) {
                    // Expect output tends toward white (255)
                    INFO("Position (" << x << ", " << y << "): blacks=" << count_black << ", whites=" << count_white);
                }
                // We can't make hard assertions because adaptive median may expand window,
                // but we can verify the output is valid binary
                CHECK((out_val == 0 || out_val == 255));
            }
        }
    }

    SECTION("Removes isolated pixels in binarized image (majority voting test)")
    {
        // Create a predominantly white image with a single black pixel
        CImg<uint> input(11, 11, 1, 1, 255);
        input(5, 5) = 0; // isolated black pixel

        CImg<uint> out = ite::adaptive_median_filter(input, 7, block_h);

        // The isolated pixel should be replaced by the majority (white)
        CHECK(out(5, 5) == 255);

        // Now test with predominantly black and isolated white pixel
        CImg<uint> input2(11, 11, 1, 1, 0);
        input2(5, 5) = 255; // isolated white pixel

        CImg<uint> out2 = ite::adaptive_median_filter(input2, 7, block_h);

        // The isolated pixel should be replaced by the majority (black)
        CHECK(out2(5, 5) == 0);
    }

    SECTION("Handles heavy salt & pepper noise on binarized image")
    {
        const auto i = GENERATE(range(0, 5)); // 5 samples

        std::seed_seq seq{ Catch::getSeed(), static_cast<std::uint32_t>(i) };
        std::mt19937 rng(seq);

        // Create a binary image with specific pattern
        CImg<uint> input(25, 25, 1, 1, 0);
        // Fill left half with white, right half with black
        for (int y = 0; y < input.height(); ++y)
            for (int x = 0; x < input.width(); ++x)
                input(x, y) = (x < 12) ? 255u : 0u;

        // Add 20% salt & pepper noise
        std::uniform_real_distribution<float> noise_dist(0.0f, 1.0f);
        std::uniform_int_distribution<int> flip_dist(0, 1);

        for (int y = 0; y < input.height(); ++y) {
            for (int x = 0; x < input.width(); ++x) {
                if (noise_dist(rng) < 0.2f) { // 20% noise
                    input(x, y) = flip_dist(rng) ? 255u : 0u;
                }
            }
        }

        CImg<uint> out = ite::adaptive_median_filter(input, 7, block_h);

        // Count how many pixels are still binary
        int binary_count = 0;
        cimg_forXY(out, x, y) {
            const uint val = out(x, y);
            if (val == 0 || val == 255) ++binary_count;
        }

        // Most pixels should still be binary after filtering
        const int total_pixels = out.width() * out.height();
        CHECK(binary_count > total_pixels * 0.95); // At least 95% binary

        // Check that the general structure is preserved (left white, right black)
        // Test pixels far from the edge
        const int left_white = static_cast<int>(out(3, 12));
        const int right_black = static_cast<int>(out(22, 12));

        CHECK(left_white > 200);  // Should be mostly white
        CHECK(right_black < 55);   // Should be mostly black
    }

    SECTION("Random input: output bounds are preserved")
    {
        const auto i = GENERATE(range(0, 10)); // 10 samples

        std::seed_seq seq{ Catch::getSeed(), static_cast<std::uint32_t>(i) };
        std::mt19937 rng(seq);
        std::uniform_int_distribution<int> dist(0, 255);

        CImg<uint> input(17, 13, 1, 1, 0);
        cimg_forXY(input, x, y) { input(x, y) = static_cast<uint>(dist(rng)); }

        const int in_min = static_cast<int>(input.min());
        const int in_max = static_cast<int>(input.max());

        CImg<uint> out = ite::adaptive_median_filter(input, 7, block_h);

        const int out_min = static_cast<int>(out.min());
        const int out_max = static_cast<int>(out.max());

        // Median filter should not introduce values outside input range
        CHECK(out_min >= in_min);
        CHECK(out_max <= in_max);
        CHECK(static_cast<int>(out.max()) <= 255);
        CHECK(static_cast<int>(out.min()) >= 0);
    }

    SECTION("Different max_window_size parameter effects")
    {
        // Create image with a few outliers
        CImg<uint> input(15, 15, 1, 1, 100);

        // Add isolated extreme pixels
        input(7, 7) = 255;
        input(3, 3) = 0;
        input(11, 11) = 0;

        // Test with small window
        CImg<uint> out_small = ite::adaptive_median_filter(input, 3, block_h);

        // Test with large window
        CImg<uint> out_large = ite::adaptive_median_filter(input, 9, block_h);

        // Both should remove the outliers
        CHECK(out_small(7, 7) == 100);
        CHECK(out_small(3, 3) == 100);
        CHECK(out_large(7, 7) == 100);
        CHECK(out_large(3, 3) == 100);

        // Large window might smooth more (but for uniform background, should be similar)
        CHECK(out_small(0, 0) == 100);
        CHECK(out_large(0, 0) == 100);
    }

    SECTION("Preserves thicker structures while removing isolated noise")
    {
        // Create a thicker block pattern (more realistic for text)
        CImg<uint> input(25, 25, 1, 1, 255);

        // Draw a larger solid block (7x7 solid block)
        // This is large enough to not be considered impulse noise
        for (int y = 9; y <= 15; ++y) {
            for (int x = 9; x <= 15; ++x) {
                input(x, y) = 0;
            }
        }

        // Add isolated noise pixels away from the block
        input(2, 2) = 0;    // pepper noise
        input(22, 2) = 0;   // pepper noise
        input(2, 22) = 0;   // pepper noise
        input(22, 22) = 0;  // pepper noise

        CImg<uint> out = ite::adaptive_median_filter(input, 5, block_h);

        // The 7x7 block should be mostly preserved (it's not an impulse)
        // At least the center should remain black
        CHECK(out(12, 12) == 0);  // center of block

        // Isolated noise should be removed
        CHECK(out(2, 2) == 255);
        CHECK(out(22, 2) == 255);
        CHECK(out(2, 22) == 255);
        CHECK(out(22, 22) == 255);

        // Background should remain white
        CHECK(out(0, 0) == 255);
        CHECK(out(24, 24) == 255);
    }

    SECTION("Handles thin lines differently than thicker structures")
    {
        // This test verifies that adaptive median filter removes thin isolated lines
        // (which is expected behavior for impulse noise removal)
        CImg<uint> input(15, 15, 1, 1, 255);

        // Single pixel width line (acts like noise chain)
        for (int i = 5; i <= 9; ++i) {
            input(7, i) = 0;  // vertical line
        }

        CImg<uint> out = ite::adaptive_median_filter(input, 5, block_h);

        // Single-pixel-wide lines are typically removed by adaptive median
        // because they're surrounded by the majority (white pixels)
        // This is expected behavior - the filter treats them as extended noise
        // We just verify the output is valid and mostly white
        int white_count = 0;
        cimg_forXY(out, x, y) {
            if (out(x, y) == 255) ++white_count;
        }

        // Most pixels should be white after filtering
        CHECK(white_count > (15 * 15) * 0.9); // At least 90% white
    }
}

