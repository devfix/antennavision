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

pos_t ScalarField::argmax_line_abs(pos_t const& pos_a, pos_t const& pos_b, double const wavelength) const
{
    pos_t const delta = pos_b - pos_a;
    math::OptimizationParams const params{[this, &pos_a, &delta, wavelength](double const x) -> double { return -std::abs(field(pos_a + x * delta, wavelength)); }, 0.0, 1.0, num_params};
    auto [x_min, f_min] = math::f_min(params);
    return pos_a + x_min * delta;
}

pos_t ScalarField::argmax_circle_abs(pos_t const& pos_center, pos_t dir_normal, double radius, pos_t const& dir_start, double angle, double wavelength) const
{
    //             ^ v2 axis (90°)
    //             |
    //         . . | . .
    //       .     |     .             ⊙ normal (up in the circle's plane)
    //     .       |       .
    //    .        |        .
    //   .         |         .
    //   .         |         .
    // --.---------C---------.---------> v1 axis (0°, start_direction)
    //   .       (Center)    .
    //    .        |        .
    //     .       |       .
    //       .     |     .
    //         . . | . .
    //             |
    dir_normal = dir_normal.normalize();

    // Project start_direction onto the plane to ensure it's perfectly perpendicular to the normal Vector projection: v_plane = v - (v . n) * n
    pos_t const v1 = (dir_start - dir_start.dot(dir_normal) * dir_normal).normalize();
    pos_t const v2 = dir_normal.cross(v1);

    math::OptimizationParams const params{[&](double const x) -> double
                                          {
                                              pos_t const pos = pos_center + radius * (std::cos(x) * v1 + std::sin(x) * v2);
                                              return -std::abs(field(pos, wavelength));
                                          },
                                          0.0, angle, num_params};
    auto [x_min, f_min] = math::f_min(params);
    return pos_center + radius * (std::cos(x_min) * v1 + std::sin(x_min) * v2);
}
