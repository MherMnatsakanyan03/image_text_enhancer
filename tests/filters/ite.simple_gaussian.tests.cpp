#include "ite.h"
#include <CImg.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("gaussian_denoise: Applies Gaussian blur", "[ite][denoise]")
{
    SECTION("Diffuses energy from a single hot pixel (Impulse Response)")
    {
        // GIVEN: A 5x5 black image with one white pixel in the center
        CImg<uint> input(5, 5, 1, 1, 0); // Fill with 0
        input(2, 2) = 255; // Set center to max

        // WHEN: We apply a gaussian blur with sigma 1.0
        CImg<uint> output = ite::simple_gaussian_blur(input, 1.0f);

        // THEN: The center value decreases and neighbors increase
        // The energy spreads out like a bell curve
        CHECK(output(2, 2) < 255); // Center lost energy to neighbors
        // Check whether the sum is approximately conserved
        double total_output = output.sum();

        CHECK(total_output > 200); // Ensure we retained most of the energy
        CHECK(total_output <= 255); // Energy should not increase

        // Check that immediate neighbors received energy
        CHECK(output(1, 2) > 0); // Left
        CHECK(output(3, 2) > 0); // Right
        CHECK(output(2, 1) > 0); // Top
        CHECK(output(2, 3) > 0); // Bottom

        // Check that corners (further away) have less energy than direct neighbors
        // (This confirms the "Gaussian" shape, falling off with distance)
        CHECK(output(0, 0) < output(1, 2));
    }

    SECTION("Does not change a uniform image")
    {
        // GIVEN: An image where every pixel is gray (100)
        // Blurring a flat color should result in the same flat color
        CImg<uint> input(5, 5, 1, 1, 100);

        // WHEN: We apply a strong blur
        CImg<uint> output = ite::simple_gaussian_blur(input, 5.0f, 1);

        // THEN: The image should remain identical
        CHECK(output(0, 0) == 100);
        CHECK(output(2, 2) == 100);
        CHECK(output(4, 4) == 100);
    }

    SECTION("Sigma 0 produces no change")
    {
        // GIVEN: A checkerboard-like image
        CImg<uint> input(3, 3, 1, 1, 0, 255, 0, 255, 0, 255, 0, 255, 0);

        // WHEN: We blur with sigma 0
        CImg<uint> output = ite::simple_gaussian_blur(input, 0.0f, 1);

        // THEN: Output should match input exactly
        CHECK(output(0, 0) == 0);
        CHECK(output(0, 1) == 255);
        CHECK(output(1, 1) == 0);
    }
}
