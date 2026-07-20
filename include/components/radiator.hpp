//
// Created by Tristan Krause on 2026-04-28.
//

#pragma once

#include <functional>
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

/**
 * Class "Radiator" of Aggregate Type
 * Also known as POD (Plain Old Data) / PDS (Passive Data Structure) / DTO (Data Transfer Object)
 */
struct Radiator
{
    using elv_spherical_t =
        std::function<vec_t(double polar, double azimuth, double wavelength)>; /// effective length vector in spherical coordinates from spherical position
    using ms_elv_t = std::function<double(double wavelength)>; /// mean-squared effective length
    static double constexpr HERTZIAN_DIPOLE_LENGTH = 1e-6;

    // Provide the ELV and mean-squared ELV functions for the Hertzian dipole
    struct HertzianDipole
    {
        [[nodiscard]] static Radiator create(std::string const& id, std::string const& origin_id);
        [[nodiscard]] static elv_spherical_t::result_type elv_spherical(double polar, double azimuth, double wavelength);
        [[nodiscard]] static ms_elv_t::result_type ms_elv(double wavelength);
    };

    // Provide the ELV and mean-squared ELV functions for the standing wave dipoles
    struct StandingWaveDipole
    {
        [[nodiscard]] static Radiator create(std::string const& id, std::string const& origin_id, double dipole_length);
        [[nodiscard]] static elv_spherical_t::result_type elv_spherical(double polar, double azimuth, double wavelength, double dipole_length);
        [[nodiscard]] static ms_elv_t::result_type ms_elv(double wavelength, double dipole_length);
    };

    [[nodiscard]] static vec_t get_elv_spherical_standing_wave(double dipole_length, double wavelength, double polar);
    [[nodiscard]] static double calc_mean_squared_effective_length(elv_spherical_t const& elv_spherical, math::NumParams const& num_params);

    [[nodiscard]] vec_t get_elv_spherical_from_cartesian(pos_t const& pos_local, double wavelength) const;

    [[nodiscard]] double calc_directivity_from_spherical(double polar, double azimuth, math::NumParams const& num_params) const;
    [[nodiscard]] double calc_directivity_from_cartesian(pos_t const& pos_local, math::NumParams const& num_params) const;

    std::string id; /// identifier name
    std::string origin_id; /// name of the origin reference
    elv_spherical_t elv_spherical{}; /// callback for effective length vector in spherical coordinates, mendatory
    ms_elv_t mean_squared_elv{}; /// callback for mean-squared effective length. Optional, can be nullptr

    // last argument since optional for brace-initializer list
    Reference* origin{};
};
