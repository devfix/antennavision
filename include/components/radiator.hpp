//
// Created by Tristan Krause on 2026-04-28.
//

#pragma once

#include <functional>
#include "components/component.hpp"
#include "reference.hpp"

// Coordinate System
// -----------------
//               +Z (Antenna Axis)
//                      ^
//                      |
//               .---.  |  .---.
//             .'     '.|.'     '.
//            /         |         \
//           |          #          |
// +X <------|----------#----------|----------
//           |          #  Y=0     |
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
    EffLenFn el_theta;
    EffLenFn el_phi;

    [[nodiscard]] virtual Vec3 calc_polar_effective_length(double theta, double phi) const = 0;
    [[nodiscard]] Vec3 calc_polar_effective_length(Vec3 const& pos_local) const;
    [[nodiscard]] virtual double calc_polar_effective_length_norm(double theta, double phi) const;
    [[nodiscard]] double calc_polar_effective_length_norm(Vec3 const& pos_local) const;
    std::complex<double> calc_path(std::size_t idx_input, std::size_t idx_output) override;
    [[nodiscard]] virtual std::complex<double> calc_radiation_gain(Vec3 const &pos, double freq) const = 0;

    [[nodiscard]] double calc_radiation_resistance(std::size_t n_theta = 101, std::size_t n_phi = 201) const;
    [[nodiscard]] double calc_directivity(double theta, double phi, std::size_t n_theta = 101, std::size_t n_phi = 201) const;
    [[nodiscard]] double calc_directivity(Vec3 const& pos_local) const;


    [[nodiscard]] std::complex<double> calc_voltage_gain(Radiator const& radiator, double lambda, std::size_t n_theta = 101, std::size_t n_phi = 201) const;
    [[nodiscard]] double calc_power_gain(Radiator const& radiator, double lambda) const;
};
