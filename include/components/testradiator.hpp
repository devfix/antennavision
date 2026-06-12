//
// Created by Tristan Krause on 2026-04-28.
//

#pragma once

#include "radiator.hpp"
#include "types.hpp"


struct TestRadiator : Radiator {
    TestRadiator(std::string_view id, Reference const& origin);

    [[nodiscard]] complex_t calc_radiation_gain(Vec3 const& pos, double freq) const override;
};
