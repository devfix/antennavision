//
// Created by Tristan Krause on 2026-06-03.
//

#pragma once

#include "types.hpp"

namespace geometry
{
    double angle_between_vectors(Vec3 vec1, Vec3 vec2);
    Quaternion quaternion_from_directions(Vec3 dir_initial, Vec3 dir_target);

    /**
     * polar to complex number
     * @param mag magnitude in Euler's plane
     * @param phi angle in Euler's plane
     * @return complex number
     */
    [[nodiscard]] constexpr complex_t ptoc(double const mag, double const phi) { return {mag * std::cos(phi), mag * std::sin(phi)}; }
} // namespace geometry
