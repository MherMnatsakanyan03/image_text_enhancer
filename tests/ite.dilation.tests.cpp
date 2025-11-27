#include "ite.h"
#include <CImg.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("dilation: Thickens bright regions", "[ite][dilation]")
{
    SECTION("Shrinks a 3x3 black hole inside a 5x5 white grid (Kernel Size 3)")
    {
        // GIVEN: A 5x5 image, mostly white (255)
        // With a 3x3 black square (0) in the middle
        /*
           W W W W W
           W B B B W
           W B B B W
           W B B B W
           W W W W W
        */
        CImg<uint> input(5, 5, 1, 1, 255); // Fill with White
        // Draw the black 3x3 box in the center (from 1,1 to 3,3)
        for (int x = 1; x <= 3; ++x)
        {
            for (int y = 1; y <= 3; ++y)
            {
                input(x, y) = 0;
            }
        }

        // Verify setup
        REQUIRE(input(2, 2) == 0);   // Center is black
        REQUIRE(input(1, 1) == 0);   // Inner corner is black
        REQUIRE(input(0, 0) == 255); // Outer border is white

        // WHEN: We dilate with kernel size 3 (Radius 1)
        // This means any black pixel touching a white pixel becomes white.
        CImg<uint> output = ite::dilation(input, 3);

        // THEN: The black hole should shrink to just the single center pixel
        /*
           W W W W W
           W W W W W
           W W B W W   <-- Only (2,2) remains black because its neighbors were all black
           W W W W W
           W W W W W
        */

        // The center (2,2) has no white neighbors in the input, so it stays black
        CHECK(output(2, 2) == 0);

        // The inner ring (e.g., 1,1) HAD white neighbors (at 0,0), so it becomes white
        CHECK(output(1, 1) == 255);
        CHECK(output(1, 2) == 255);
        CHECK(output(2, 1) == 255);
        CHECK(output(3, 3) == 255);
    }

    SECTION("Expands a single white pixel (Impulse Response) with Kernel 3")
    {
        // GIVEN: A 5x5 black image with one white pixel in the center
        CImg<uint> input(5, 5, 1, 1, 0);
        input(2, 2) = 255;

        // WHEN: Dilate with Kernel 3
        CImg<uint> output = ite::dilation(input, 3);

        // THEN: The single pixel should become a 3x3 white block
        // Center stays white
        CHECK(output(2, 2) == 255);

        // Immediate neighbors become white
        CHECK(output(1, 1) == 255);
        CHECK(output(1, 2) == 255);
        CHECK(output(1, 3) == 255);
        CHECK(output(2, 1) == 255);
        CHECK(output(2, 3) == 255);
        CHECK(output(3, 1) == 255);
        CHECK(output(3, 2) == 255);
        CHECK(output(3, 3) == 255);

        // Rest stay black
        CHECK(output(0, 0) == 0);
        CHECK(output(0, 1) == 0);
        CHECK(output(0, 2) == 0);
        CHECK(output(0, 3) == 0);
        CHECK(output(0, 4) == 0);
        CHECK(output(1, 0) == 0);
        CHECK(output(1, 4) == 0);
        CHECK(output(2, 0) == 0);
        CHECK(output(2, 4) == 0);
        CHECK(output(3, 0) == 0);
        CHECK(output(3, 4) == 0);
        CHECK(output(4, 0) == 0);
        CHECK(output(4, 1) == 0);
        CHECK(output(4, 2) == 0);
        CHECK(output(4, 3) == 0);
        CHECK(output(4, 4) == 0);
    }

    SECTION("Expands a single white pixel with larger Kernel 5")
    {
        // GIVEN: A 7x7 black image with one white pixel in the center
        CImg<uint> input(7, 7, 1, 1, 0);
        input(3, 3) = 255;

        // WHEN: Dilate with Kernel 5 (Radius 2)
        CImg<uint> output = ite::dilation(input, 5);

        // THEN: The single pixel should become a 5x5 white block

        // Distance 2 pixels (e.g., 1,1) should now be white
        CHECK(output(1, 1) == 255);
        CHECK(output(5, 5) == 255);

        // Distance 3 pixels (0,0) should still be black
        CHECK(output(0, 0) == 0);
    }
}
