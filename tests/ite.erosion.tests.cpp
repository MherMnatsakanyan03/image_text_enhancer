#include "ite.h"
#include <CImg.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("erosion: Thickens dark regions", "[ite][erosion]")
{
    SECTION("Shrinks a 3x3 white block inside a 5x5 black grid (Kernel Size 3)")
    {
        // GIVEN: A 5x5 image, mostly black (0)
        // With a 3x3 white square (255) in the middle
        /*
           B B B B B
           B W W W B
           B W W W B
           B W W W B
           B B B B B
        */
        CImg<uint> input(5, 5, 1, 1, 0); // Fill with Black
        // Draw the white 3x3 box in the center
        for(int x=1; x<=3; ++x) {
            for(int y=1; y<=3; ++y) {
                input(x,y) = 255; 
            }
        }

        // Verify setup
        REQUIRE(input(2, 2) == 255); 
        REQUIRE(input(1, 1) == 255); 
        REQUIRE(input(0, 0) == 0); 

        // WHEN: We erode with kernel size 3 (Radius 1)
        // Any white pixel touching a black pixel becomes black.
        CImg<uint> output = ite::erosion(input, 3);

        // THEN: The white block should shrink to just the single center pixel
        /*
           B B B B B
           B B B B B
           B B W B B   <-- Only (2,2) remains white because its neighbors were all white
           B B B B B
           B B B B B
        */

        // The center (2,2) had only white neighbors, so it stays white
        CHECK(output(2, 2) == 255); 

        // The inner ring (e.g., 1,1) had black neighbors (at 0,0), so it gets eaten (black)
        CHECK(output(1, 1) == 0); 
        CHECK(output(1, 2) == 0);
        CHECK(output(1, 3) == 0);
        CHECK(output(2, 1) == 0);
        CHECK(output(2, 3) == 0);
        CHECK(output(3, 1) == 0);
        CHECK(output(3, 2) == 0);
        CHECK(output(3, 3) == 0);
    }

    SECTION("Removes a single white pixel (Noise Removal) with Kernel 3")
    {
        // GIVEN: A 5x5 black image with one white pixel (noise)
        CImg<uint> input(5, 5, 1, 1, 0); 
        input(2, 2) = 255; 

        // WHEN: Erode with Kernel 3
        // Since the white pixel has black neighbors, it should be removed.
        CImg<uint> output = ite::erosion(input, 3);

        // THEN: The image should be completely black
        CHECK(output(2, 2) == 0);
        
        // Verify sum is 0 (image is empty)
        long long total_sum = 0;
        cimg_forXY(output, x, y) { total_sum += output(x, y); }
        CHECK(total_sum == 0);
    }

    SECTION("Large white block shrinks with Kernel 5")
    {
        // GIVEN: A 7x7 white image with a black border
        CImg<uint> input(7, 7, 1, 1, 255); 
        // Paint border black manually
        cimg_forXY(input, x, y) {
            if (x==0 || x==6 || y==0 || y==6) input(x,y) = 0;
        }

        // We have a 5x5 white block inside.
        REQUIRE(input(3, 3) == 255);
        REQUIRE(input(1, 1) == 255); // Edge of white block
        
        // WHEN: Erode with Kernel 5 (Radius 2)
        // This eats 2 pixels from every side.
        CImg<uint> output = ite::erosion(input, 5);

        // THEN: The 5x5 white block shrinks by 2 on all sides -> 1x1 white block
        
        // Center survives
        CHECK(output(3, 3) == 255);
        
        // The edge of the previous white block (1,1) is consumed
        CHECK(output(1, 1) == 0);
        // Even the next pixel in (2,2) is consumed (distance 2 from black edge)
        CHECK(output(2, 2) == 0);
    }
}
