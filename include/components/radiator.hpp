//
// Created by Tristan Krause on 2026-04-28.
//

#pragma once

#include <functional>

#include "components/component.hpp"
#include "math.hpp"
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
    using elv_spherical_t = std::function<vec_t(double polar, double azimuth, double wavelength)>; /// effective length vector in spherical coordinates from spherical position
    using ms_elv_t = std::function<double(double wavelength)>; /// mean-squared effective length
    static double constexpr HERTZIAN_DIPOLE_LENGTH = 1e-3;

    // Provide the elv and msel functions for the Hertzian dipole
    struct HertzianDipole
    {
        [[nodiscard]] static Radiator create(std::string_view id, Reference const& origin);
        [[nodiscard]] static elv_spherical_t::result_type elv_spherical(double polar, double azimuth, double wavelength);
        [[nodiscard]] static ms_elv_t::result_type msel(double wavelength);
    };

    // Provide the elv and msel functions for the standing wave dipoles
    struct StandingWaveDipole
    {
        [[nodiscard]] static Radiator create(std::string_view id, Reference const& origin, double dipole_length);
        [[nodiscard]] static elv_spherical_t::result_type elv_spherical(double polar, double azimuth, double wavelength, double dipole_length);
        [[nodiscard]] static ms_elv_t::result_type msel(double wavelength, double dipole_length);
    };

    Radiator(std::string_view id, Reference const& origin, elv_spherical_t elv_spherical, ms_elv_t msel = nullptr);

    Radiator(Radiator const&) = delete; // disable copy constructor
    Radiator& operator=(Radiator const&) = delete; // disable copy assignment
    Radiator(Radiator&&) = delete; // disable move constructor
    Radiator& operator=(Radiator&&) = delete; // disable move assignment

    Reference const& origin;
    elv_spherical_t const elv_spherical; /// callback for effective length vector in spherical coordinates
    ms_elv_t const mean_squared_elv; /// callback for mean-squared effective length. Optional, can be nullptr

    [[nodiscard]] static vec_t get_elv_spherical_standing_wave(double dipole_length, double wavelength, double azimuth);
    [[nodiscard]] static double calc_mean_squared_effective_length(elv_spherical_t const& elv_spherical, double wavelength, math::NumParams const& num_params);

    [[nodiscard]] vec_t get_elv_spherical_from_cartesian(pos_t const& pos_local, double wavelength) const;

    [[nodiscard]] double calc_directivity_from_spherical(double polar, double azimuth, double wavelength, math::NumParams const& num_params) const;
    [[nodiscard]] double calc_directivity_from_cartesian(pos_t const& pos_local, double wavelength, math::NumParams const& num_params) const;

    [[nodiscard]] static std::complex<double> calc_voltage_gain(Radiator const& radiator_tx, Radiator const& radiator_rx, double wavelength, math::NumParams const& num_params);
    [[nodiscard]] static double calc_power_gain(Radiator const& radiator_tx, Radiator const& radiator_rx, double wavelength, math::NumParams const& num_params);

    std::complex<double> calc_path(std::size_t idx_input, std::size_t idx_output) override;
};
