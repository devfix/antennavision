//
// Created by core on 04.07.26.
//

#pragma once

#include <functional>
#include "math.hpp"
#include "types.hpp"

struct ScalarField
{
    using field_t = std::function<complex_t(pos_t const& pos, double wavelength)>;
    using reset_t = std::function<void()>;
    ScalarField(field_t&& field, reset_t&& reset, math::NumParams const& num_params);
    ~ScalarField();

    std::pair<pos_t, double> argmax_line_abs(pos_t const& pos_a, pos_t const& pos_b, double wavelength) const;

    /**
     *
     * @param pos_center
     * @param dir_normal
     * @param radius
     * @param dir_start
     * @param angle
     * @param wavelength
     * @return
     */
    std::pair<pos_t, double> argmax_circle_abs(math::Circle const& circle, double angle, double wavelength) const;

    std::pair<pos_t, double> calc_beamwidth(math::Circle const& circle, double ratio, double wavelength) const;

    field_t const field;
    math::NumParams const& num_params;

private:
    reset_t const reset;
};
