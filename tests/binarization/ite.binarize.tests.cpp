#include "ite.h"
#include <CImg.h>
#include <catch2/catch_test_macros.hpp>
#include <iostream>

TEST_CASE("binarize: Converts grayscale to binary (black/white)", "[ite][binarize][Otsu][Sauvola][Bataineh]")
{
    // Section "Converts a simple grayscale image", "Converts an RGB image implicitly" and "Handles an image that is all one color" are the same as in
    // ite.binarize.tests.cpp
    SECTION("Converts a simple grayscale image [Otsu]")
    {
        // GIVEN: A 4x1 grayscale image with a clear bimodal distribution
        // 2 pixels at 50 (dark gray), 2 pixels at 200 (light gray)
        CImg<uint> input_gray(4, 1, 1, 1, // 4x1, 1-depth, 1-channel
                              50, 50, 200, 200);

        REQUIRE(input_gray.spectrum() == 1);

        // ----------------------------------------------------------------
        // Otsu Binarization
        // ----------------------------------------------------------------
        // WHEN: We binarize the image.
        // The Otsu's threshold should be calculated as ~125.
        CImg<uint> output_otsu = ite::binarize_otsu(input_gray);

        // THEN: The output should be binary (0 or 255)
        CHECK(output_otsu.width() == 4);
        CHECK(output_otsu.spectrum() == 1);
        // Pixels < threshold (125) should be 0
        CHECK(output_otsu(0, 0) == 0); // was 50
        CHECK(output_otsu(1, 0) == 0); // was 50

        // Pixels >= threshold (125) should be 255
        CHECK(output_otsu(2, 0) == 255); // was 200
        CHECK(output_otsu(3, 0) == 255); // was 200
    }

    SECTION("Converts a simple grayscale image [Sauvola]")
    {
        // GIVEN: A 4x1 grayscale image with a clear bimodal distribution
        // 2 pixels at 50 (dark gray), 2 pixels at 200 (light gray)
        CImg<uint> input_gray(4, 1, 1, 1, // 4x1, 1-depth, 1-channel
                              50, 50, 200, 200);

        REQUIRE(input_gray.spectrum() == 1);

        // ----------------------------------------------------------------
        // Sauvola Binarization
        // ----------------------------------------------------------------
        // WHEN: We binarize the image.
        // The Sauvola's threshold should be calculated based on local mean and stddev.
        CImg<uint> output_sauvola = ite::binarize_sauvola(input_gray);

        // THEN: The output should be binary (0 or 255)
        CHECK(output_sauvola.width() == 4);
        CHECK(output_sauvola.spectrum() == 1);
        // Pixels < local threshold should be 0
        CHECK(output_sauvola(0, 0) == 0); // was 50
        CHECK(output_sauvola(1, 0) == 0); // was 50

        // Pixels >= local threshold should be 255
        CHECK(output_sauvola(2, 0) == 255); // was 200
        CHECK(output_sauvola(3, 0) == 255); // was 200
    }

    SECTION("Converts a simple grayscale image [Bataineh]")
    {
        // GIVEN: A 4x1 grayscale image with a clear bimodal distribution
        // 2 pixels at 50 (dark gray), 2 pixels at 200 (light gray)
        CImg<uint> input_gray(4, 1, 1, 1, // 4x1, 1-depth, 1-channel
                              50, 50, 200, 200);

        REQUIRE(input_gray.spectrum() == 1);

        // ----------------------------------------------------------------
        // Bataineh Binarization
        // ----------------------------------------------------------------
        // WHEN: We binarize the image.
        CImg<uint> output_bataineh = ite::binarize_bataineh(input_gray);

        // THEN: The output should be binary (0 or 255)
        CHECK(output_bataineh.width() == 4);
        CHECK(output_bataineh.spectrum() == 1);
        // Pixels < threshold (125) should be 0
        CHECK(output_bataineh(0, 0) == 0); // was 50
        CHECK(output_bataineh(1, 0) == 0); // was 50

        // Pixels >= threshold (125) should be 255
        CHECK(output_bataineh(2, 0) == 255); // was 200
        CHECK(output_bataineh(3, 0) == 255); // was 200
    }

    SECTION("Converts an RGB image implicitly [Otsu]")
    {
        // GIVEN: A 2x1 RGB image with one dark pixel and one light pixel
        // P1: (50, 50, 50) -> Grayscale luminance ~50
        // P2: (200, 200, 200) -> Grayscale luminance ~200
        CImg<uint> input_rgb(2, 1, 1, 3, // 2x1, 1-depth, 3-channel
                             50, 200, // R-plane
                             50, 200, // G-plane
                             50, 200); // B-plane

        REQUIRE(input_rgb.spectrum() == 3);

        // WHEN: We binarize the image.
        // The function should first convert to grayscale ([50, 200])
        // and then find the optimal threshold (~125).
        CImg<uint> output = ite::binarize_otsu(input_rgb);

        // THEN: The output should be 1-channel (binary)
        CHECK(output.width() == 2);
        CHECK(output.spectrum() == 1);

        CHECK(output(0, 0) == 0); // was 50
        CHECK(output(1, 0) == 255); // was 200
    }

    SECTION("Converts an RGB image implicitly [Sauvola]")
    {
        // GIVEN: A 2x1 RGB image with one dark pixel and one light pixel
        // P1: (50, 50, 50) -> Grayscale luminance ~50
        // P2: (200, 200, 200) -> Grayscale luminance ~200
        CImg<uint> input_rgb(2, 1, 1, 3, // 2x1, 1-depth, 3-channel
                             50, 200, // R-plane
                             50, 200, // G-plane
                             50, 200); // B-plane

        REQUIRE(input_rgb.spectrum() == 3);

        // WHEN: We binarize the image.
        // The function should first convert to grayscale ([50, 200])
        // and then find the local thresholds.
        CImg<uint> output = ite::binarize_sauvola(input_rgb);

        // THEN: The output should be 1-channel (binary)
        CHECK(output.width() == 2);
        CHECK(output.spectrum() == 1);

        CHECK(output(0, 0) == 0); // was 50
        CHECK(output(1, 0) == 255); // was 200
    }

    SECTION("Converts an RGB image implicitly [Bataineh]")
    {
        // GIVEN: A 2x1 RGB image with one dark pixel and one light pixel
        // P1: (50, 50, 50) -> Grayscale luminance ~50
        // P2: (200, 200, 200) -> Grayscale luminance ~200
        CImg<uint> input_rgb(2, 1, 1, 3, // 2x1, 1-depth, 3-channel
                             50, 200, // R-plane
                             50, 200, // G-plane
                             50, 200); // B-plane

        REQUIRE(input_rgb.spectrum() == 3);

        // WHEN: We binarize the image.
        // The function should first convert to grayscale ([50, 200])
        // and then find the optimal threshold (~125).
        CImg<uint> output = ite::binarize_bataineh(input_rgb);

        // THEN: The output should be 1-channel (binary)
        CHECK(output.width() == 2);
        CHECK(output.spectrum() == 1);

        CHECK(output(0, 0) == 0); // was 50
        CHECK(output(1, 0) == 255); // was 200
    }

    SECTION("Handles an image that is all one color [Otsu]")
    {
        // GIVEN: A 2x2 grayscale image of all one color
        CImg<uint> input_gray(2, 2, 1, 1, 150); // All pixels are 150

        // WHEN: We binarize the image.
        // The algorithm should find a threshold
        // and all pixels will end up on one side.
        CImg<uint> output = ite::binarize_otsu(input_gray);

        // THEN: All pixels should be the same value (either 0 or 255)
        CHECK(output.spectrum() == 1);
        uint first_pixel_val = output(0, 0);
        CHECK(output(1, 0) == first_pixel_val);
        CHECK(output(0, 1) == first_pixel_val);
        CHECK(output(1, 1) == first_pixel_val);
    }

    SECTION("Handles an image that is all one color [Sauvola]")
    {
        // GIVEN: A 2x2 grayscale image of all one color
        CImg<uint> input_gray(2, 2, 1, 1, 150); // All pixels are 150

        // WHEN: We binarize the image.
        // The algorithm should find a threshold
        // and all pixels will end up on one side.
        CImg<uint> output = ite::binarize_sauvola(input_gray);

        // THEN: All pixels should be the same value (either 0 or 255)
        CHECK(output.spectrum() == 1);
        uint first_pixel_val = output(0, 0);
        CHECK(output(1, 0) == first_pixel_val);
        CHECK(output(0, 1) == first_pixel_val);
        CHECK(output(1, 1) == first_pixel_val);
    }

    SECTION("Handles an image that is all one color [Bataineh]")
    {
        // GIVEN: A 2x2 grayscale image of all one color
        CImg<uint> input_gray(2, 2, 1, 1, 150); // All pixels are 150

        // WHEN: We binarize the image.
        // The algorithm should find a threshold
        // and all pixels will end up on one side.
        CImg<uint> output = ite::binarize_bataineh(input_gray);

        // THEN: All pixels should be the same value (either 0 or 255)
        CHECK(output.spectrum() == 1);
        uint first_pixel_val = output(0, 0);
        CHECK(output(1, 0) == first_pixel_val);
        CHECK(output(0, 1) == first_pixel_val);
        CHECK(output(1, 1) == first_pixel_val);
    }

    SECTION("High contrast separation [Otsu]")
    {
        // GIVEN: white image with black block
        CImg<uint> input_image(50, 50, 1, 1, 255); // All white
        input_image.draw_rectangle(20, 10, 30, 40, "0"); // Black square in center

        // WHEN: We binarize the image.
        CImg<uint> output = ite::binarize_otsu(input_image);

        // THEN: The black block should be 0, rest 255
        CHECK(output.spectrum() == 1);
        CHECK(output(25, 25) == 0); // Inside black block
        CHECK(output(5, 5) == 255); // Outside black block
    }

    SECTION("High contrast separation [Sauvola]")
    {
        // GIVEN: white image with black block
        CImg<uint> input_image(50, 50, 1, 1, 255); // All white
        input_image.draw_rectangle(20, 10, 30, 40, "0"); // Black square in center

        // WHEN: We binarize the image.
        CImg<uint> output = ite::binarize_sauvola(input_image);

        // THEN: The black block should be 0, rest 255
        CHECK(output.spectrum() == 1);
        CHECK(output(25, 25) == 0); // Inside black block
        CHECK(output(5, 5) == 255); // Outside black block
    }

    SECTION("High contrast separation [Bataineh]")
    {
        // GIVEN: white image with black block
        CImg<uint> input_image(50, 50, 1, 1, 255); // All white
        input_image.draw_rectangle(20, 10, 30, 40, "0"); // Black square in center

        // WHEN: We binarize the image.
        CImg<uint> output = ite::binarize_bataineh(input_image);

        // THEN: The black block should be 0, rest 255
        CHECK(output.spectrum() == 1);
        CHECK(output(25, 25) == 0); // Inside black block
        CHECK(output(5, 5) == 255); // Outside black block
    }

    SECTION("Low contrast separation [Otsu]")
    {
        // GIVEN: light gray image with slightly darker text
        CImg<uint> input_image(100, 100, 1, 1, 200); // All light gray
        input_image.draw_text(20, 40, "TEST", "190", 0, 1, 30); // Darker text in center

        // WHEN: We binarize the image.
        CImg<uint> output = ite::binarize_otsu(input_image);

        // THEN: The darker text should be 0, rest 255
        CHECK(output.spectrum() == 1);
        CImg<uint> crop = output.get_crop(20, 40, 20 + 30, 40 + 30); // Crop around text
        int black_pixel = 0;
        cimg_forXY(crop, x, y)
        {
            if (crop(x, y) == 0)
                black_pixel++;
        }
        double density = (double)black_pixel / (crop.width() * crop.height());
        // The density of black pixels should be reasonable for the text area
        CHECK(density > 0.1); // At least 10% of cropped area
        CHECK(density < 0.5); // Less than 50% of cropped area
    }

    SECTION("Low contrast separation [Sauvola]")
    {
        // GIVEN: light gray image with slightly darker text
        CImg<uint> input_image(100, 100, 1, 1, 200); // All light gray
        input_image.draw_text(20, 40, "TEST", "190", 0, 1, 30); // Darker text in center

        // WHEN: We binarize the image.
        CImg<uint> output = ite::binarize_sauvola(input_image);

        // THEN: The darker text should be 0, rest 255
        CHECK(output.spectrum() == 1);
        CImg<uint> crop = output.get_crop(20, 40, 20 + 30, 40 + 30); // Crop around text
        int black_pixel = 0;
        cimg_forXY(crop, x, y)
        {
            if (crop(x, y) == 0)
                black_pixel++;
        }
        double density = (double)black_pixel / (crop.width() * crop.height());
        // The density of black pixels should be reasonable for the text area
        CHECK(density > 0.1); // At least 10% of cropped area
        CHECK(density < 0.5); // Less than 50% of cropped area
    }

    SECTION("Low contrast separation [Bataineh]")
    {
        // GIVEN: light gray image with slightly darker text
        CImg<uint> input_image(100, 100, 1, 1, 200); // All light gray
        input_image.draw_text(20, 40, "TEST", "190", 0, 1, 30); // Darker text in center

        // WHEN: We binarize the image.
        CImg<uint> output = ite::binarize_bataineh(input_image);

        // THEN: The darker text should be 0, rest 255
        CHECK(output.spectrum() == 1);
        CImg<uint> crop = output.get_crop(20, 40, 20 + 30, 40 + 30); // Crop around text
        int black_pixel = 0;
        cimg_forXY(crop, x, y)
        {
            if (crop(x, y) == 0)
                black_pixel++;
        }
        double density = (double)black_pixel / (crop.width() * crop.height());
        // The density of black pixels should be reasonable for the text area
        CHECK(density > 0.1); // At least 10% of cropped area
        CHECK(density < 0.5); // Less than 50% of cropped area
    }
}

TEST_CASE("binarize: Edge cases", "[ite][binarize][Otsu][Sauvola][Bataineh]")
{
    SECTION("Solid images should not cause crashes (division by zero) [Otsu]")
    {
        // GIVEN: A solid color image (all pixels the same)
        CImg<uint> solid_image_dark(10, 10, 1, 1, 0); // All pixels are 0
        CImg<uint> solid_image_light(10, 10, 1, 1, 255); // All pixels are 255

        // WHEN: We binarize the image.
        // THEN: It should not crash because of division by zero.
        REQUIRE_NOTHROW(ite::binarize_otsu(solid_image_dark));
        REQUIRE_NOTHROW(ite::binarize_otsu(solid_image_light));
    }

    SECTION("Solid images should not cause crashes (division by zero) [Sauvola]")
    {
        // GIVEN: A solid color image (all pixels the same)
        CImg<uint> solid_image_dark(10, 10, 1, 1, 0); // All pixels are 0
        CImg<uint> solid_image_light(10, 10, 1, 1, 255); // All pixels are 255

        // WHEN: We binarize the image.
        // THEN: It should not crash because of division by zero.
        REQUIRE_NOTHROW(ite::binarize_sauvola(solid_image_dark));
        REQUIRE_NOTHROW(ite::binarize_sauvola(solid_image_light));
    }

    SECTION("Solid images should not cause crashes (division by zero) [Bataineh]")
    {
        // GIVEN: A solid color image (all pixels the same)
        CImg<uint> solid_image_dark(10, 10, 1, 1, 0); // All pixels are 0
        CImg<uint> solid_image_light(10, 10, 1, 1, 255); // All pixels are 255

        // WHEN: We binarize the image.
        // THEN: It should not crash because of division by zero.
        REQUIRE_NOTHROW(ite::binarize_bataineh(solid_image_dark));
        REQUIRE_NOTHROW(ite::binarize_bataineh(solid_image_light));
    }

    SECTION("Small images are handled correctly relative to window size [Otsu]")
    {
        // GIVEN: A 5x5 grayscale image
        CImg<uint> tiny_image(5, 5, 1, 1, 150); // Single pixel with value 150
        tiny_image(2, 2) = 0; // Center pixel is black

        // WHEN: We binarize the image, with default window size of 15.
        CImg<uint> output = ite::binarize_otsu(tiny_image);

        // THEN: The black pixel should be 0, others 255
        CHECK(output.spectrum() == 1);
        CHECK(output(2, 2) == 0);
        CHECK(output(0, 0) == 255);
    }

    SECTION("Small images are handled correctly relative to window size [Sauvola]")
    {
        // GIVEN: A 5x5 grayscale image
        CImg<uint> tiny_image(5, 5, 1, 1, 150); // Single pixel with value 150
        tiny_image(2, 2) = 0; // Center pixel is black

        // WHEN: We binarize the image, with default window size of 15.
        CImg<uint> output = ite::binarize_sauvola(tiny_image);

        // THEN: The black pixel should be 0, others 255
        CHECK(output.spectrum() == 1);
        CHECK(output(2, 2) == 0);
        CHECK(output(0, 0) == 255);
    }

    SECTION("Small images are handled correctly relative to window size [Bataineh]")
    {
        // GIVEN: A 5x5 grayscale image
        CImg<uint> tiny_image(5, 5, 1, 1, 150); // Single pixel with value 150
        tiny_image(2, 2) = 0; // Center pixel is black

        // WHEN: We binarize the image, with default window size of 15.
        CImg<uint> output = ite::binarize_bataineh(tiny_image);

        // THEN: The black pixel should be 0, others 255
        CHECK(output.spectrum() == 1);
        CHECK(output(2, 2) == 0);
        CHECK(output(0, 0) == 255);
    }
}