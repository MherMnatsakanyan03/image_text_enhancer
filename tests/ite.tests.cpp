#include "ite.h"
#include <CImg.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("to_grayscale: Converts RGB to 1-channel grayscale", "[ite][grayscale]") {
    // GIVEN: A small, known RGB CImg object
    // WHEN: ite::to_grayscale() is called
    // THEN: The output image should have 1 channel and correct pixel values
    REQUIRE(true); // Placeholder assertion
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