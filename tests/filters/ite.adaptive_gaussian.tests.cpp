#include "ite.h"
#include <CImg.h>
#include <algorithm>
#include <cmath>
#include <random>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "catch2/catch_get_random_seed.hpp"
#include "catch2/generators/catch_generators.hpp"
#include "catch2/generators/catch_generators_range.hpp"

TEST_CASE("adaptive_gaussian_blur: behaves correctly in extremes and preserves edges", "[ite][denoise]")
{
    constexpr int truncate = 3;
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

        CImg<uint> out = input;

        ite::adaptive_gaussian_blur(out, 1.0f, 4.0f, 50.0f, truncate, block_h);

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

    SECTION("edge_thresh == 0 forces LOW blur everywhere (a == 1)")
    {
        // Make something non-uniform so low/high differ.
        CImg<uint> input(9, 9, 1, 1, 0);
        for (int y = 0; y < input.height(); ++y)
            for (int x = 0; x < input.width(); ++x)
                input(x, y) = ((x + y) % 2) ? 255u : 0u; // checkerboard

        // Reference low/high
        CImg<uint> low = input;
        CImg<uint> high = input;
        ite::simple_gaussian_blur(low, 1.0f, truncate);
        ite::simple_gaussian_blur(high, 3.0f, truncate);

        // Adaptive with edge_thresh=0 => should equal low exactly
        CImg<uint> out = input;
        ite::adaptive_gaussian_blur(out, 1.0f, 3.0f, 0.0f, truncate, block_h);

        cimg_forXY(out, x, y) { CHECK(out(x, y) == low(x, y)); }
    }

    SECTION("Very large edge_thresh forces HIGH blur everywhere (a ~= 0)")
    {
        CImg<uint> input(9, 9, 1, 1, 0);
        for (int y = 0; y < input.height(); ++y)
            for (int x = 0; x < input.width(); ++x)
                input(x, y) = (x < 4) ? 30u : 220u; // step edge

        // Add mild checker noise so blur has something to do
        for (int y = 0; y < input.height(); ++y)
            for (int x = 0; x < input.width(); ++x)
            {
                int v = (int)input(x, y) + (((x + y) % 2) ? 8 : -8);
                input(x, y) = (uint)std::clamp(v, 0, 255);
            }

        CImg<uint> high = input;
        ite::simple_gaussian_blur(high, 3.0f, truncate);

        CImg<uint> out = input;
        ite::adaptive_gaussian_blur(out, 1.0f, 3.0f, /*edge_thresh*/ 1e9f, truncate, block_h);

        cimg_forXY(out, x, y) { CHECK(out(x, y) == high(x, y)); }
    }

    SECTION("Random input: output bounds and monotonicity")
    {
        const auto i = GENERATE(range(0, 10)); // 10 samples

        std::seed_seq seq{ Catch::getSeed(), static_cast<std::uint32_t>(i) };
        std::mt19937 rng(seq);
        std::uniform_int_distribution<int> dist(0, 255);

        CImg<uint> input(17, 13, 1, 1, 0);
        cimg_forXY(input, x, y) { input(x, y) = static_cast<uint>(dist(rng)); }

        auto out = input;
        ite::adaptive_gaussian_blur(out, 1.0f, 4.0f, 50.0f, truncate, block_h);

        CHECK(static_cast<int>(out.max()) <= 255);

        const int in_min  = static_cast<int>(input.min());
        const int in_max  = static_cast<int>(input.max());
        const int out_min = static_cast<int>(out.min());
        const int out_max = static_cast<int>(out.max());

        CHECK(out_min > in_min - 5);
        CHECK(out_max < in_max + 5);
    }
}
