#include "ite.h"
#include <CImg.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("binarize_inplace: Converts grayscale to binary (black/white)", "[ite][binarize_inplace]")
{
    SECTION("Converts a simple grayscale image")
    {
        // GIVEN: A 4x1 grayscale image with a clear bimodal distribution
        // 2 pixels at 50 (dark gray), 2 pixels at 200 (light gray)
        CImg<uint> input_gray(4, 1, 1, 1, // 4x1, 1-depth, 1-channel
                              50, 50, 200, 200);

        REQUIRE(input_gray.spectrum() == 1);

        // WHEN: We binarize the image.
        // The Otsu's threshold should be calculated as ~125.
        CImg<uint> output = ite::binarize_otsu(input_gray);

        // THEN: The output should be binary (0 or 255)
        CHECK(output.width() == 4);
        CHECK(output.spectrum() == 1);

        // Pixels < threshold (125) should be 0
        CHECK(output(0, 0) == 0); // was 50
        CHECK(output(1, 0) == 0); // was 50

        // Pixels >= threshold (125) should be 255
        CHECK(output(2, 0) == 255); // was 200
        CHECK(output(3, 0) == 255); // was 200
    }

    SECTION("Converts an RGB image implicitly")
    {
        // GIVEN: A 2x1 RGB image with one dark pixel and one light pixel
        // P1: (50, 50, 50) -> Grayscale luminance ~50
        // P2: (200, 200, 200) -> Grayscale luminance ~200
        CImg<uint> input_rgb(2, 1, 1, 3, // 2x1, 1-depth, 3-channel
                             50, 200, // R-plane
                             50, 200, // G-plane
                             50, 200); // B-plane

        REQUIRE(input_rgb.spectrum() == 3);

        // WHEN: We binarize the image.
        // The function should first convert to grayscale ([50, 200])
        // and then find the optimal threshold (~125).
        CImg<uint> output = ite::binarize_sauvola(input_rgb);

        // THEN: The output should be 1-channel (binary)
        CHECK(output.width() == 2);
        CHECK(output.spectrum() == 1);

        CHECK(output(0, 0) == 0); // was 50
        CHECK(output(1, 0) == 255); // was 200
    }

    SECTION("Handles an image that is all one color")
    {
        // GIVEN: A 2x2 grayscale image of all one color
        CImg<uint> input_gray(2, 2, 1, 1, 150); // All pixels are 150

        // WHEN: We binarize the image.
        // Otsu's algorithm should find a threshold (e.g., 0 or 150)
        // and all pixels will end up on one side.
        CImg<uint> output = ite::binarize_otsu(input_gray);

        // THEN: All pixels should be the same value (either 0 or 255)
        CHECK(output.spectrum() == 1);
        uint first_pixel_val = output(0, 0);
        CHECK(output(1, 0) == first_pixel_val);
        CHECK(output(0, 1) == first_pixel_val);
        CHECK(output(1, 1) == first_pixel_val);
    }
}
