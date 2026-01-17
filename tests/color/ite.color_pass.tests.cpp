#include "ite.h"
#include <CImg.h>
#include <catch2/catch_test_macros.hpp>

// Define 'uint' for clarity matching your library
using uint = unsigned int;

TEST_CASE("color_pass: Applies binary mask to color image", "[ite][color]")
{
    SECTION("Overlays a binary mask onto a green color image (Green Plus Symbol)")
    {
        // GIVEN: A 5x5 Green Color Image
        // We set Red=0, Green=255, Blue=0
        CImg<uint> color_img(5, 5, 1, 3, 0);
        cimg_forXY(color_img, x, y)
        {
            color_img(x, y, 0, 0) = 0; // R
            color_img(x, y, 0, 1) = 255; // G
            color_img(x, y, 0, 2) = 0; // B
        }

        // AND: A 5x5 Binary Mask with a Black 'Plus' symbol on White background
        // W W W W W
        // W W B W W
        // W B B B W
        // W W B W W
        // W W W W W
        // (W=255 Background, B=0 Foreground/Text)
        CImg<uint> bin_img(5, 5, 1, 1, 255); // Fill with White (Background)

        // Draw the Plus (Text/Foreground = 0)
        bin_img(2, 1) = 0; // Top
        bin_img(1, 2) = 0; // Left
        bin_img(2, 2) = 0; // Center
        bin_img(3, 2) = 0; // Right
        bin_img(2, 3) = 0; // Bottom

        // WHEN: color_pass is applied
        // The binary mask tells us: "Where it is White, wipe the color to White. Where it is Black, keep the Green."
        CImg<uint> output = ite::color_pass(bin_img, color_img);

        // THEN: The output should be a Green Plus on a White Background.

        // 1. Check Background Pixel (0,0) -> Should be White (255, 255, 255)
        // because bin_img(0,0) was 255.
        CHECK(output(0, 0, 0, 0) == 255);
        CHECK(output(0, 0, 0, 1) == 255);
        CHECK(output(0, 0, 0, 2) == 255);

        // 2. Check Foreground Pixel (2,2) - Center of Plus -> Should be Green (0, 255, 0)
        // because bin_img(2,2) was 0.
        CHECK(output(2, 2, 0, 0) == 0);
        CHECK(output(2, 2, 0, 1) == 255);
        CHECK(output(2, 2, 0, 2) == 0);

        // 3. Check Foreground Pixel (1,2) - Left arm -> Should be Green
        CHECK(output(1, 2, 0, 0) == 0);
        CHECK(output(1, 2, 0, 1) == 255);
        CHECK(output(1, 2, 0, 2) == 0);
    }

    SECTION("Throws exception for mismatched dimensions")
    {
        // GIVEN: Images of different sizes
        CImg<uint> color_img(10, 10, 1, 3);
        CImg<uint> bin_img(5, 5, 1, 1);

        // WHEN/THEN: Calling color_pass throws invalid_argument
        REQUIRE_THROWS_AS(ite::color_pass(bin_img, color_img), std::invalid_argument);
    }

    SECTION("Throws exception for incorrect channel counts")
    {
        // Case 1: Binary mask is not 1-channel (e.g., 3 channels)
        CImg<uint> color_img(5, 5, 1, 3);
        CImg<uint> bin_img_3ch(5, 5, 1, 3);
        REQUIRE_THROWS_AS(ite::color_pass(bin_img_3ch, color_img), std::invalid_argument);

        // Case 2: Color image is not 3-channel (e.g., 1 channel)
        CImg<uint> color_img_1ch(5, 5, 1, 1);
        CImg<uint> bin_img(5, 5, 1, 1);
        REQUIRE_THROWS_AS(ite::color_pass(bin_img, color_img_1ch), std::invalid_argument);
    }

    SECTION("Handles empty images gracefully")
    {
        // GIVEN: Empty images
        CImg<uint> empty_color;
        CImg<uint> empty_bin;

        // WHEN: color_pass is called
        // The implementation should check for empty inputs and return immediately (usually an empty image)
        CImg<uint> result = ite::color_pass(empty_bin, empty_color);

        // THEN: Result should be empty
        CHECK(result.is_empty());
    }
}
