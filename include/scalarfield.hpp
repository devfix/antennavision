//
// Created by core on 04.07.26.
//

#pragma once

#include "math.hpp"
#include "types.hpp"
#include <functional>

struct ScalarField
{
    using field_t = std::function<complex_t(pos_t pos, double wavelength, math::NumParams const& num_params)>;
    ScalarField(field_t const& field, math::NumParams const& num_params);
    field_t const field;
    math::NumParams const& num_params;
};
