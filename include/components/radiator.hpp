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
    using elv_spherical_t = std::function<nc::NdArray<complex_t>(double polar, double azimuth, double wavelength)>; /// effective length vector in spherical coordinates
    using msel_t = std::function<double(double wavelength)>;  /// mean-squared effective length
    static double constexpr HERTZIAN_DIPOLE_LENGTH = 1e-3;

    // Provide the elv and msel functions for the Hertzian dipole
    struct HertzianDipole
    {
        [[nodiscard]] static Radiator create(std::string_view id, Reference const &origin);
        [[nodiscard]] static elv_spherical_t::result_type elv_spherical(double polar, double azimuth, double wavelength);
        [[nodiscard]] static msel_t::result_type msel(double wavelength);
    };

    Radiator(std::string_view id, Reference const &origin, elv_spherical_t  elv_spherical, msel_t  msel = nullptr);

    Radiator(Radiator const &) = delete; // disable copy constructor
    Radiator &operator=(Radiator const &) = delete; // disable copy assignment
    Radiator(Radiator &&) = delete; // disable move constructor
    Radiator &operator=(Radiator &&) = delete; // disable move assignment

    Reference const &origin;
    elv_spherical_t const elv_spherical; /// callback for effective length vector in spherical coordinates
    msel_t const msel; /// callback for mean-squared effective length. Optional, can be nullptr

    [[nodiscard]] static nc::NdArray<complex_t> get_elv_spherical_standing_wave(double dipole_length, double wavelength, double polar, double phase_i);
    [[nodiscard]] static double calc_mean_squared_effective_length(elv_spherical_t const& elv_spherical, double wavelength, std::size_t n_azimuth = 101, std::size_t n_polar = 201);

    // [[nodiscard]] virtual Vec3 calc_polar_effective_length(double azimuth, double polar) const = 0;
    [[nodiscard]] nc::NdArray<complex_t> calc_elv_spherical_from_cartesian(Vec3 const& pos_local, double wavelength) const;
    // [[nodiscard]] virtual double calc_polar_effective_length_norm(double azimuth, double polar) const;
    // [[nodiscard]] double calc_polar_effective_length_norm(Vec3 const& pos_local) const;
    std::complex<double> calc_path(std::size_t idx_input, std::size_t idx_output) override;
    // [[nodiscard]] virtual std::complex<double> calc_radiation_gain(Vec3 const &pos, double freq) const = 0;
    [[nodiscard]] double calc_radiation_resistance(std::size_t n_azimuth = 101, std::size_t n_polar = 201) const;
    [[nodiscard]] double calc_directivity(double azimuth, double polar, double wavelength, std::size_t n_azimuth = 101, std::size_t n_polar = 201) const;
    [[nodiscard]] double calc_directivity(Vec3 const& pos_local, double wavelength) const;


    [[nodiscard]] std::complex<double> calc_voltage_gain(Radiator const& radiator, double wavelength, std::size_t n_azimuth = 101, std::size_t n_polar = 201) const;
    [[nodiscard]] double calc_power_gain(Radiator const& radiator, double wavelength) const;
};
