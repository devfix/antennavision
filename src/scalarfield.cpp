//
// Created by core on 04.07.26.
//

#include "scalarfield.hpp"
#include <ansi_color.hpp>
#include <nlopt.hpp>
#include <utility>

ScalarField::ScalarField(field_t&& field, reset_t&& reset, math::NumParams const& num_params) : field(std::move(field)), num_params(num_params), reset(std::move(reset)) {}

ScalarField::~ScalarField()
{
    if (reset) { reset(); }
}

std::pair<pos_t, double> ScalarField::argmax_line_abs(pos_t const& pos_a, pos_t const& pos_b, double const wavelength) const
{
    pos_t const delta = pos_b - pos_a;
    math::OptParams const params{[this, &pos_a, &delta, wavelength](double const x) -> double { return -std::abs(field(pos_a + x * delta, wavelength)); }, 0.0, 1.0, num_params};
    auto [x_min, f_min] = math::f_min(params);
    pos_t pos_max = pos_a + x_min * delta;
    return {pos_max, std::abs(field(pos_max, wavelength))};
}

std::pair<pos_t, double> ScalarField::argmax_circle_abs(math::Circle const& circle, double const angle, double const wavelength) const
{
    math::OptParams const params{[&](double const x) -> double
                                 {
                                     pos_t const pos = circle.center + circle.radius * (std::cos(x) * circle.v1 + std::sin(x) * circle.v2);
                                     return -std::abs(field(pos, wavelength));
                                 },
                                 0.0, angle, num_params};
    auto [angle_min, f_min] = math::f_min(params);
    pos_t pos_max = circle.center + circle.radius * (std::cos(angle_min) * circle.v1 + std::sin(angle_min) * circle.v2);
    return {pos_max, std::abs(field(pos_max, wavelength))};
}

std::pair<pos_t, double> ScalarField::calc_beamwidth(math::Circle const& circle, double const ratio, double const wavelength) const
{
    math::Circle circle_rot(circle);
    circle_rot.rotate_base(-pi / 4.0);
    auto [pos_beam, intensity] = argmax_circle_abs(circle_rot, pi / 2.0, wavelength);

    auto circle_hpbw = math::get_circle(circle.center, circle.normal, circle.radius, pos_beam - circle.center);
    math::OptParams const params{[&](double const x) -> double
                                 {
                                     pos_t const pos = circle_hpbw.center + circle_hpbw.radius * (std::cos(x) * circle_hpbw.v1 + std::sin(x) * circle_hpbw.v2);
                                     return math::square(std::abs(field(pos, wavelength)) - ratio * intensity);
                                 },
                                 0.0, pi / 4.0, num_params};
    auto [angle1, eps1] = math::f_min(params);

    circle_hpbw.rotate_base(-pi / 4.0);
    auto [angle2_inv, eps2] = math::f_min(params);
    double angle2 = pi/4.0 - angle2_inv;

    return {pos_beam, angle1 + angle2};
}
