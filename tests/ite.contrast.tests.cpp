#include "ite.h"
#include <CImg.h>
#include <catch2/catch_test_macros.hpp>

using uint = unsigned int;

TEST_CASE("contrast_enhancement: Stretches histogram", "[ite][contrast]")
{
    SECTION("Stretches a low-contrast image (washed out gray)")
    {
        // GIVEN: An image with values only between 100 and 150
        // (Simulates faint text on gray paper)
        CImg<uint> input(10, 10, 1, 1, 0);
        
        // Fill halves with 100 and 150
        for(int i=0; i<50; ++i) input[i] = 100;
        for(int i=50; i<100; ++i) input[i] = 150;

        // Verify initial range
        REQUIRE(input.min() == 100);
        REQUIRE(input.max() == 150);

        // WHEN: Enhance contrast
        CImg<uint> output = ite::contrast_enhancement(input);

        // THEN: The range should expand towards 0 and 255
        // The 100s should become dark (near 0)
        // The 150s should become bright (near 255)
        CHECK(output.min() < 10);
        CHECK(output.max() > 245);
    }

    SECTION("Ignores Outliers (Robustness Test)")
    {
        // GIVEN: An image range [100, 150] PLUS single outliers at 0 and 255.
        // A standard normalization would see 0 and 255 and do NOTHING.
        // Your robust algorithm (1% clip) should ignore them and stretch the [100, 150] part.
        
        CImg<uint> input(10, 10, 1, 1, 125); // Mostly mid-gray
        
        // Add a block of "darker" gray (100) and "lighter" gray (150)
        // These represent the actual content we want to see.
        for(int i=0; i<40; ++i) input[i] = 100;
        for(int i=40; i<80; ++i) input[i] = 150;

        // Add the outliers (Salt and Pepper noise) - only 2 pixels out of 100 (2%)
        // Note: Your algorithm clips 1% from bottom and 1% from top.
        // 1 pixel = 1% of 100. So 1 pixel is exactly on the boundary.
        input(9, 8) = 0;   // One black dot
        input(9, 9) = 255; // One white dot

        // Verify setup: Full range exists strictly because of noise
        REQUIRE(input.min() == 0);
        REQUIRE(input.max() == 255);

        // WHEN: Enhance contrast
        CImg<uint> output = ite::contrast_enhancement(input);

        // THEN: The "content" (100 and 150) should still be stretched!
        // We pick a pixel that used to be 100. It should now be much darker.
        CHECK(output(0, 0) < 50); 

        // We pick a pixel that used to be 150. It should now be much brighter.
        CHECK(output(5, 5) > 200);
    }
}