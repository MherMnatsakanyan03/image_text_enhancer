#pragma once
/**
 * @file types.h
 * @brief Common type definitions and constants for the ITE library.
 */

#include <cstdint>
#include "CImg.h"

// Use the CImg library namespace
using namespace cimg_library;

namespace ite
{

    // Standard luminance weights (Rec. 601)
    constexpr float WEIGHT_R = 0.299f;
    constexpr float WEIGHT_G = 0.587f;
    constexpr float WEIGHT_B = 0.114f;

    // Rec. 709 luminance weights (alternative, used in HD video)
    constexpr float WEIGHT_R_709 = 0.2126f;
    constexpr float WEIGHT_G_709 = 0.7152f;
    constexpr float WEIGHT_B_709 = 0.0722f;

} // namespace ite
