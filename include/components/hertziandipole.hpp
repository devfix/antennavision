//
// Created by Tristan Krause on 2026-06-08.
//

#pragma once
#include "radiator.hpp"

struct HertzianDipole : Radiator
{
    HertzianDipole(std::string_view id, Reference const& origin, double length);

    double const length;  /// length of the Hertzian dipole, must be small compared to the wave length

    [[nodiscard]] Vec3 calc_el_polar(double theta, double phi) const override;
    [[nodiscard]] complex_t calc_radiation_gain(Vec3 const& pos, double freq) const override;
};