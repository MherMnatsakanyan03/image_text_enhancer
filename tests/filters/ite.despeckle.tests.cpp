#include "ite.h"
#include <CImg.h>
#include <catch2/catch_test_macros.hpp>

// Define 'uint' for clarity matching your library
using uint = unsigned int;

TEST_CASE("despeckle: Removes small connected components", "[ite][despeckle]")
{
    SECTION("Removes specks smaller than threshold (5px, 4px, 3px scenario)")
    {
        // GIVEN: A blank white image (Background = 255)
        // We will draw 3 black specks (Foreground = 0)
        CImg<uint> input(20, 20, 1, 1, 255);

        // 1. Create a 5-pixel speck (A horizontal line) at top-left
        // Coordinates: (0,0) to (4,0)
        for (int x = 0; x < 5; ++x)
            input(x, 0) = 0;

        // 2. Create a 4-pixel speck (A 2x2 square) in the middle
        // Coordinates: (10,10) to (11,11)
        input(10, 10) = 0;
        input(11, 10) = 0;
        input(10, 11) = 0;
        input(11, 11) = 0;

        // 3. Create a 3-pixel speck (A vertical line) at bottom-right
        // Coordinates: (15,15) to (15,17)
        for (int y = 15; y < 18; ++y)
            input(15, y) = 0;

        // Verify setup
        REQUIRE(input(0, 0) == 0); // Speck 1 exists
        REQUIRE(input(10, 10) == 0); // Speck 2 exists
        REQUIRE(input(15, 15) == 0); // Speck 3 exists

        // WHEN: We despeckle with threshold = 5.
        // Logic: Remove if size < 5.
        // Size 3 < 5 -> Remove.
        // Size 4 < 5 -> Remove.
        // Size 5 < 5 -> False (Keep).
        CImg<uint> output = ite::despeckle(input, 5, true);

        // THEN: Only the 5-pixel speck should remain

        // Speck 1 (5px) - Should be Black (0)
        CHECK(output(0, 0) == 0);
        CHECK(output(4, 0) == 0);

        // Speck 2 (4px) - Should be White (255) / Gone
        CHECK(output(10, 10) == 255);
        CHECK(output(11, 11) == 255);

        // Speck 3 (3px) - Should be White (255) / Gone
        CHECK(output(15, 15) == 255);
        CHECK(output(15, 17) == 255);
    }

    SECTION("Handles diagonal connections correctly")
    {
        // GIVEN: A diagonal line of 3 pixels
        // B . .
        // . B .
        // . . B
        // On a slightly larger grid to avoid boundary issues
        CImg<uint> input(5, 5, 1, 1, 255);
        input(1, 1) = 0;
        input(2, 2) = 0;
        input(3, 3) = 0;

        // WHEN: Despeckle with threshold 2

        // CASE A: Diagonal Connections = TRUE (8-way)
        // The pixels connect to form one object of size 3.
        // 3 < 2 is FALSE -> Object is KEPT.
        CImg<uint> output_diag = ite::despeckle(input, 2, true);

        // CASE B: Diagonal Connections = FALSE (4-way)
        // The pixels do NOT connect. They are 3 objects of size 1.
        // 1 < 2 is TRUE -> Objects are REMOVED.
        CImg<uint> output_nodiag = ite::despeckle(input, 2, false);

        // THEN
        // In Diagonal mode, pixels should remain black (0)
        CHECK(output_diag(1, 1) == 0);
        CHECK(output_diag(2, 2) == 0);

        // In No-Diagonal mode, pixels should become white (255)
        CHECK(output_nodiag(1, 1) == 255);
        CHECK(output_nodiag(2, 2) == 255);
    }
}