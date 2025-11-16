#include "ite.h"
#include <CImg.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("to_grayscale: Converts RGB to 1-channel grayscale", "[ite][grayscale]") {
    SECTION("Converts a 3-channel RGB image") {
        // GIVEN: A 2x1 pixel, 3-channel (RGB) image
        // Pixel (0,0) is Red (255, 0, 0)
        // Pixel (1,0) is Green (0, 255, 0)
        // The values are specified by plane: (R-plane, G-plane, B-plane)
        CImg<unsigned char> input_rgb(2, 1, 1, 3,  // 2x1, 1-depth, 3-channel
                                      255, 0,      // R-plane: [Red, Green]
                                      0, 255,      // G-plane: [Red, Green]
                                      0, 0);       // B-plane: [Red, Green]

        REQUIRE(input_rgb.width() == 2);
        REQUIRE(input_rgb.spectrum() == 3);

        // WHEN: We convert it to grayscale
        CImg<unsigned char> output = ite::to_grayscale(input_rgb);

        // THEN: The output should be 1-channel with correct luminance values
        // Formula: 0.299*R + 0.587*G + 0.114*B

        CHECK(output.width() == 2);
        CHECK(output.height() == 1);
        CHECK(output.spectrum() == 1);

        // Check Pixel (0,0) - Red (255,0,0)
        // 0.299*255 + 0.587*0 + 0.114*0 = 76.245
        INFO("INFOINFOINFO " + output(0, 0));
        CHECK(output(0, 0) == static_cast<unsigned char>(std::round(76.245f))); // 76

        // Check Pixel (1,0) - Green (0,255,0)
        // 0.299*0 + 0.587*255 + 0.114*0 = 149.685
        CHECK(output(1, 0) == static_cast<unsigned char>(std::round(149.685f))); // 150
    }

    SECTION("Does nothing to an already 1-channel image") {
        // GIVEN: A 1x1, 1-channel (grayscale) image
        CImg<unsigned char> input_gray(1, 1, 1, 1, 128); // 1x1, 1-depth, 1-channel, value 128

        REQUIRE(input_gray.spectrum() == 1);
        REQUIRE(input_gray(0, 0) == 128);

        // WHEN: We call the function
        CImg<unsigned char> output = ite::to_grayscale(input_gray);

        // THEN: The output image should be an identical copy
        CHECK(output.width() == 1);
        CHECK(output.spectrum() == 1);
        CHECK(output(0, 0) == 128);
    }
}

TEST_CASE("binarize: Converts grayscale to binary (black/white)", "[ite][binarize]") {
    // GIVEN: A small grayscale CImg with values above and below a threshold
    // WHEN: ite::binarize() is called
    // THEN: The output pixels should be either 0 or 255
    REQUIRE(true); // Placeholder assertion
}

TEST_CASE("gaussian_denoise: Applies Gaussian blur", "[ite][denoise]") {
    // GIVEN: A small CImg with a single "hot" pixel (impulse)
    // WHEN: ite::gaussian_denoise() is called
    // THEN: The center pixel value should decrease, and neighbors should increase
    REQUIRE(true); // Placeholder assertion
}

TEST_CASE("dilation: Thickens bright regions", "[ite][dilation]") {
    // GIVEN: A small binary CImg with a single bright pixel
    // WHEN: ite::dilation() is called
    // THEN: The neighboring pixels should also become bright
    REQUIRE(true); // Placeholder assertion
}

TEST_CASE("erosion: Thickens dark regions", "[ite][erosion]") {
    // GIVEN: A small binary CImg that is all bright except one dark pixel
    // WHEN: ite::erosion() is called
    // THEN: The neighboring pixels should also become dark
    REQUIRE(true); // Placeholder assertion
}

TEST_CASE("deskew: Corrects image rotation", "[ite][deskew]") {
    // GIVEN: A synthetic grayscale CImg with a clearly rotated line
    // WHEN: ite::deskew() is called
    // THEN: The output image's rotation angle should be 0 (or very close)
    REQUIRE(true); // Placeholder assertion
}

TEST_CASE("contrast_enhancement: Stretches histogram", "[ite][contrast]") {
    // GIVEN: A "low-contrast" grayscale CImg (e.g., all pixels between 100 and 150)
    // WHEN: ite::contrast_enhancement() is called
    // THEN: The output image's min value should be 0 and max value 255
    REQUIRE(true); // Placeholder assertion
}