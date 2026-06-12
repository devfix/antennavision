//
// Created by Tristan Krause on 2026-04-29.
//


#pragma once

#include <complex>
#include "radiator.hpp"


struct IsotropicRadiator : Radiator {
    IsotropicRadiator(std::string_view id, Reference const& origin);

    [[nodiscard]] complex_t calc_radiation_gain(Vec3 const& pos, double freq) const override;
};
