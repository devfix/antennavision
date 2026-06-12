//
// Created by Tristan Krause on 2026-04-28.
//

#pragma once

#include <functional>
#include "components/component.hpp"
#include "reference.hpp"

// Coordinate System
// -----------------
//               +X (Antenna Axis)
//                      ^
//                      |
//               .---.  |  .---.
//             .'     '.|.'     '.
//            /         |         \
//           |          #          |
// +Y <------|----------#----------|----------
//           |          #  z=0     |
//            \         |         /
//             '.     '.|.'     .'
//               '---'  |  '---'
//                      |
//                      |
//
// Rotation Convention
// -------------------
// NumCpp / Physics Convention / East-North-Up (ENU)
//  +Z points Up (Toward the sky)
//  +Y points Forward/Left
//  +X points Right/Forward
// In the standard convention
// - yaw is around the vertical axis (Z)
// - pitch is around the lateral axis (Y)
// - roll is around the longitudinal axis (X)

struct Radiator : Component
{
    using EffLenFn = std::function<double(double, double)>;
    Radiator(std::string_view id, Reference const &origin);

    Radiator(Radiator const &) = delete; // disable copy constructor
    Radiator &operator=(Radiator const &) = delete; // disable copy assignment
    Radiator(Radiator &&) = delete; // disable move constructor
    Radiator &operator=(Radiator &&) = delete; // disable move assignment

    Reference const &origin;
    EffLenFn const el_theta;
    EffLenFn const el_phi;

    [[nodiscard]] virtual Vec3 calc_el_polar(double theta, double phi) const = 0;
    [[nodiscard]] virtual double calc_el_norm_polar(double theta, double phi) const;
    complex_t calc_path(std::size_t idx_input, std::size_t idx_output) override;
    [[nodiscard]] virtual complex_t calc_radiation_gain(Vec3 const &pos, double freq) const = 0;

    [[nodiscard]] double calc_directivity(double theta, double phi, std::size_t n_theta = 1001, std::size_t n_phi = 2001) const;
};
