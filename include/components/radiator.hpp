//
// Created by Tristan Krause on 2026-04-28.
//

#pragma once

#include <functional>
#include "reference.hpp"
#include "setup/simparams.hpp"

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
        std::function<Vec(double polar, double azimuth, double wavelength)>; /// effective length vector in spherical coordinates from spherical position
    using ms_elv_t = std::function<double(double wavelength)>; /// mean-squared effective length
    static double constexpr HERTZIAN_DIPOLE_LENGTH = 1e-6; /// ideally infinitely small, we use 1um that is less than any reasonable wavelength
    static double constexpr ISOTROPIC_RADIATOR_LENGTH = 1.0; /// arbitrary reference length (simulation artefact), will cancel-out in the gain calculation

    /// Provide the ELV and mean-squared ELV functions for the isotropic radiator
    struct IsotropicRadiator
    {
        [[nodiscard]] static Radiator create(std::string const& id, std::string const& origin_id);
        [[nodiscard]] static Vec elv_spherical(double polar, double azimuth, double wavelength);
        [[nodiscard]] static double ms_elv(double wavelength);
    };

    /// Provide the ELV and mean-squared ELV functions for the Hertzian dipole
    struct HertzianDipole
    {
        [[nodiscard]] static Radiator create(std::string const& id, std::string const& origin_id);
        [[nodiscard]] static Vec elv_spherical(double polar, double azimuth, double wavelength);
        [[nodiscard]] static double ms_elv(double wavelength);
    };

    /// Provide the ELV and mean-squared ELV functions for the standing wave dipoles
    struct StandingWaveDipole
    {
        [[nodiscard]] static Radiator create(std::string const& id, std::string const& origin_id, double dipole_length);
        [[nodiscard]] static Vec elv_spherical(double polar, double azimuth, double wavelength, double dipole_length);
        [[nodiscard]] static double ms_elv(double wavelength, double dipole_length);
    };

    // [[nodiscard]] static Vec get_elv_spherical_standing_wave(double dipole_length, double wavelength, double polar);
    [[nodiscard]] static double calc_mean_squared_effective_length(elv_spherical_t const& elv_spherical, double wavelength, setup::SimParams const& sim_params);

    [[nodiscard]] Vec get_elv_spherical_from_cartesian(Pos const& pos_local, double wavelength) const;

    [[nodiscard]] double calc_directivity_from_spherical(double polar, double azimuth, double wavelength, setup::SimParams const& sim_params) const;
    [[nodiscard]] double calc_directivity_from_cartesian(Pos const& pos_local, double wavelength, setup::SimParams const& sim_params) const;

    std::string id; /// identifier name
    std::string origin_id; /// name of the origin reference
    elv_spherical_t elv_spherical{}; /// callback for effective length vector in spherical coordinates, mandatory
    ms_elv_t mean_squared_elv{}; /// callback for mean-squared effective length. Optional, can be nullptr

    // last argument since optional for brace-initializer list
    reference::Reference* origin{};
};
