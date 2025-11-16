#include "ite.h"
#include <CImg.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("gaussian_denoise: Applies Gaussian blur", "[ite][denoise]")
{
    // GIVEN: A small CImg with a single "hot" pixel (impulse)
    // WHEN: ite::gaussian_denoise() is called
    // THEN: The center pixel value should decrease, and neighbors should increase
    REQUIRE(true); // Placeholder assertion
}
