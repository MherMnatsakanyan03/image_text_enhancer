#pragma once
/**
 * @file utils.h
 * @brief Utility functions (clamping, etc.) for the ITE library.
 * All functions are inline for header-only usage.
 */

#include <algorithm>
#include <cmath>

namespace ite::utils
{

    /**
     * @brief Clamps an integer value to the range [lo, hi].
     */
    inline int clampi(int v, int lo, int hi) { return std::clamp(v, lo, hi); }

    /**
     * @brief Clamps a float value to the range [lo, hi].
     */
    inline float clampf(float v, float lo, float hi) { return std::clamp(v, lo, hi); }

    /**
     * @brief Clamps a uint value to [0, 255] range.
     */
    inline unsigned char clamp_to_u8(uint v) { return (v > 255u) ? 255u : static_cast<unsigned char>(v); }

    /**
     * @brief Clamps a float value to [0, 255] and rounds to uint.
     */
    inline uint clamp_float_to_u8(float v)
    {
        int iv = static_cast<int>(std::lround(v));
        if (iv < 0)
            return 0u;
        if (iv > 255)
            return 255u;
        return static_cast<uint>(iv);
    }

} // namespace ite::utils
