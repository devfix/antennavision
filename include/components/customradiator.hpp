//
// Created by Tristan Krause on 2026-06-08.
//

#pragma once
#include "radiator.hpp"

struct CustomRadiator : Radiator
{
    CustomRadiator(std::string_view id, Reference const& origin, std::function<Vec3(double, double)> &&effective_length);

    std::function<Vec3(double, double)> const effective_length;

    [[nodiscard]] Vec3 calc_polar_effective_length(double theta, double phi) const override;
    [[nodiscard]] complex_t calc_radiation_gain(Vec3 const& pos, double freq) const override;
};