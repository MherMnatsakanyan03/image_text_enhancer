#include "ite.h"
#include <CImg.h>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

TEST_CASE("deskew: Corrects image rotation", "[ite][deskew]")
{
    SECTION("Corrects a rotated line")
    {
        // GIVEN: An image with a single horizontal line (simulating a line of text)
        // 100x100 Black background
        CImg<uint> input(100, 100, 1, 1, 0);
        
        // Draw a horizontal white line (thicker: 3px) in the middle
        // (From x=20 to x=80 at y=49, 50, 51)
        for (int x = 20; x < 80; ++x) {
            input(x, 49) = 255;
            input(x, 50) = 255;
            input(x, 51) = 255;
        }

        // We purposely rotate it by a known angle (e.g., 10 degrees)
        // 1 = Linear interpolation, 0 = Fill with black
        CImg<uint> skewed = input.get_rotate(10, 1, 0);

        // Verify it is actually skewed (center row sum should be lower than original)
        // Original line length approx 60 * 3 * 255
        double original_max_row_sum = 0;
        // The middle row of the original line should be fully white
        for (int x = 0; x < 100; ++x) original_max_row_sum += input(x, 50);
        
        double skewed_center_row_sum = 0;
        for (int x = 0; x < skewed.width(); ++x) skewed_center_row_sum += skewed(x, skewed.height()/2);

        // The skew smears the line across multiple rows, so the max row sum drops significantly
        REQUIRE(skewed_center_row_sum < original_max_row_sum * 0.7);

        // WHEN: We deskew the image
        CImg<uint> output = ite::deskew(skewed);

        // THEN: The line should be horizontal again.
        // We verify this by checking the Projection Profile (Variance of row sums).
        
        // Calculate Variance of skewed image
        double sum_sq_skew = 0, sum_skew = 0;
        cimg_forY(skewed, y) {
            double r = 0; cimg_forX(skewed, x) r += skewed(x, y);
            sum_skew += r; sum_sq_skew += r*r;
        }
        double var_skew = (sum_sq_skew / skewed.height()) - std::pow(sum_skew / skewed.height(), 2);

        // Calculate Variance of deskewed (output) image
        double sum_sq_out = 0, sum_out = 0;
        cimg_forY(output, y) {
            double r = 0; cimg_forX(output, x) r += output(x, y);
            sum_out += r; sum_sq_out += r*r;
        }
        double var_out = (sum_sq_out / output.height()) - std::pow(sum_out / output.height(), 2);

        // The deskewed image should have significantly higher variance (sharper profile)
        CHECK(var_out > var_skew * 1.2);
    }

    SECTION("Leaves an already straight image mostly unchanged")
    {
        // GIVEN: A perfectly horizontal line
        CImg<uint> input(100, 100, 1, 1, 0);
        for (int x = 20; x < 80; ++x) {
            input(x, 49) = 255;
            input(x, 50) = 255;
            input(x, 51) = 255;
        }

        // WHEN: Deskew is called
        CImg<uint> output = ite::deskew(input);

        // THEN: It shouldn't rotate it unnecessarily.
        // We check if the pixel at the geometric center is still white.
        // We use output.width()/2 because rotate() changes image dimensions.
        int cx = output.width() / 2;
        int cy = output.height() / 2;

        INFO("Output Dimensions: " << output.width() << "x" << output.height());
        
        // Allow for minor interpolation artifacts, but main structure should hold.
        // Center pixel should be part of the white line.
        CHECK(output(cx, cy) > 200); 
        
        // Pixel slightly below center should be black background.
        // (Original line was at 49, 50, 51. So 50+5 = 55 is safely background).
        CHECK(output(cx, cy + 5) < 50); 
    }
}