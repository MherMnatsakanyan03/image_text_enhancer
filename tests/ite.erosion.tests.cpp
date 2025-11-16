#include "ite.h"
#include <CImg.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("erosion: Thickens dark regions", "[ite][erosion]")
{
    // GIVEN: A small binary CImg that is all bright except one dark pixel
    // WHEN: ite::erosion() is called
    // THEN: The neighboring pixels should also become dark
    REQUIRE(true); // Placeholder assertion
}
