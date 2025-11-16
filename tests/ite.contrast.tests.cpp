#include "ite.h"
#include <CImg.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("contrast_enhancement: Stretches histogram", "[ite][contrast]")
{
    // GIVEN: A "low-contrast" grayscale CImg (e.g., all pixels between 100 and 150)
    // WHEN: ite::contrast_enhancement() is called
    // THEN: The output image's min value should be 0 and max value 255
    REQUIRE(true); // Placeholder assertion
}