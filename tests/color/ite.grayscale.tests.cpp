#include "ite.h"
#include <CImg.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("to_grayscale: Converts RGB to 1-channel grayscale", "[ite][grayscale]")
{
    SECTION("Converts a 3-channel RGB image")
    {
        // GIVEN: A 2x1 pixel, 3-channel (RGB) image
        // Pixel (0,0) is Red (255, 0, 0)
        // Pixel (1,0) is Green (0, 255, 0)
        // The values are specified by plane: (R-plane, G-plane, B-plane)
        CImg<uint> input_rgb(2, 1, 1, 3, // 2x1, 1-depth, 3-channel
                             255, 0, // R-plane: [Red, Green]
                             0, 255, // G-plane: [Red, Green]
                             0, 0); // B-plane: [Red, Green]

        REQUIRE(input_rgb.width() == 2);
        REQUIRE(input_rgb.spectrum() == 3);

        // WHEN: We convert it to grayscale
        CImg<uint> output = ite::to_grayscale(input_rgb);

        // THEN: The output should be 1-channel with correct luminance values
        // Formula: 0.299*R + 0.587*G + 0.114*B

        CHECK(output.width() == 2);
        CHECK(output.height() == 1);
        CHECK(output.spectrum() == 1);

        // Check Pixel (0,0) - Red (255,0,0)
        // 0.299*255 + 0.587*0 + 0.114*0 = 76.245
        CHECK(output(0, 0) == static_cast<uint>(std::round(76.245f))); // 76

        // Check Pixel (1,0) - Green (0,255,0)
        // 0.299*0 + 0.587*255 + 0.114*0 = 149.685
        CHECK(output(1, 0) == static_cast<uint>(std::round(149.685f))); // 150
    }

    SECTION("Does nothing to an already 1-channel image")
    {
        // GIVEN: A 1x1, 1-channel (grayscale) image
        CImg<uint> input_gray(1, 1, 1, 1, 128); // 1x1, 1-depth, 1-channel, value 128

        REQUIRE(input_gray.spectrum() == 1);
        REQUIRE(input_gray(0, 0) == 128);

        // WHEN: We call the function
        CImg<uint> output = ite::to_grayscale(input_gray);

        // THEN: The output image should be an identical copy
        CHECK(output.width() == 1);
        CHECK(output.spectrum() == 1);
        CHECK(output(0, 0) == 128);
    }
}
