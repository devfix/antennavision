//
// Created by Tristan Krause on 2026-04-28.
//

#include "components/radiator.hpp"
#include <NumCpp/Functions/linspace.hpp>
#include <NumCpp/Functions/meshgrid.hpp>
#include <NumCpp/Functions/sin.hpp>
#include <NumCpp/Functions/sum.hpp>
#include <print>
#include <string>
#include "factory/get.hpp"
#include "factory/make.hpp"
#include "math/coords.hpp"
#include "math/functions.hpp"

Radiator Radiator::IsotropicRadiator::create(std::string const& id, std::string const& origin_id)
{
    return {
        .type = Type::IsotropicRadiator,
        .id = id,
        .origin_id = origin_id,
        .elv_spherical = nullptr,
        .ms_elv = nullptr //
    };
}

Radiator Radiator::HertzianDipole::create(std::string const& id, std::string const& origin_id)
{
    return {
        .type = Type::HertzianDipole,
        .id = id,
        .origin_id = origin_id,
        .elv_spherical = elv_spherical,
        .ms_elv = ms_elv //
    };
}

Radiator::elv_spherical_t::result_type Radiator::HertzianDipole::elv_spherical(double polar, double, double)
{ return math::vec<Complex>(0, -HERTZIAN_DIPOLE_LENGTH * std::sin(polar), 0); }

Radiator::ms_elv_t::result_type Radiator::HertzianDipole::ms_elv(double) { return 2.0 / 3.0 * math::square(HERTZIAN_DIPOLE_LENGTH); }

Radiator Radiator::StandingWaveDipole::create(std::string const& id, std::string const& origin_id, double dipole_length)
{
    return {
        .type = Type::StandingWaveDipole,
        .id = id,
        .origin_id = origin_id,
        .elv_spherical = [dipole_length](double polar, [[maybe_unused]] double azimuth, double wavelength) -> elv_spherical_t::result_type
        { return elv_spherical(polar, azimuth, wavelength, dipole_length); },
        .ms_elv = [dipole_length](double wavelength) -> ms_elv_t::result_type
        {
            return ms_elv(wavelength, dipole_length);
        } //
    };
}

Vec Radiator::StandingWaveDipole::elv_spherical(double polar, [[maybe_unused]] double azimuth, double wavelength, double dipole_length)
{
    double x = pi * dipole_length / wavelength;
    Complex const polar_comp = -wavelength / (pi * std::sin(polar)) * (std::cos(x * std::cos(polar)) - cos(x));
    return math::vec<Complex>(0, polar_comp, 0);
}

double Radiator::StandingWaveDipole::ms_elv(double wavelength, double dipole_length)
{
    // Half-Wave-Dipole: n=1, Full-Wave-Dipole: n=2, ...
    double n = dipole_length / (0.5 * wavelength);
    return 0.5 * math::square(wavelength / pi) * math::q_function(n * pi);
}

double Radiator::calc_ms_elv(elv_spherical_t const& elv_spherical, double wavelength, setup::SimParams const& sim_params)
{
    sim_params.assert_integrity();
    auto const n_polar = static_cast<std::int32_t>(sim_params.n_polar);
    auto const n_azimuth = static_cast<std::int32_t>(sim_params.n_azimuth);

    auto const polar_edges = nc::linspace(0.0, pi, n_polar + 1);
    auto const azimuth_edges = nc::linspace(0.0, 2.0 * pi, n_azimuth + 1);
    auto const d_polar = pi / static_cast<double>(sim_params.n_polar);
    auto const d_azimuth = 2.0 * pi / static_cast<double>(sim_params.n_azimuth);

    auto const polar_mids = 0.5 *
        (polar_edges(polar_edges.rSlice(), nc::Slice(0, n_polar)) //
            + polar_edges(polar_edges.rSlice(), nc::Slice(1, n_polar + 1)));
    auto const azimuth_mids = 0.5 *
        (azimuth_edges(azimuth_edges.rSlice(), nc::Slice(0, n_azimuth)) //
            + azimuth_edges(azimuth_edges.rSlice(), nc::Slice(1, n_azimuth + 1)));
    auto const [polar_grid, azimuth_grid] = nc::meshgrid(polar_mids, azimuth_mids);

    RealArray squared_norms(polar_grid.shape());
    std::ranges::transform(polar_grid,
        azimuth_grid,
        squared_norms.begin(),
        [&elv_spherical, wavelength](double polar, double azimuth) -> double
        {//
            return math::square(math::norm(elv_spherical(polar, azimuth, wavelength)));
        });

    // Reshape squared_norms back to match the grid shape (num_azimuth x num_polar)
    squared_norms = squared_norms.reshape(polar_grid.shape());

    // Compute the integrand: || l_e ||^2 * sin(polar)
    auto const integrand = squared_norms * nc::sin(polar_grid);

    double integral = nc::sum(integrand).item() * d_polar * d_azimuth;
    return integral / (4.0 * pi);
}

Vec Radiator::get_elv_spherical_from_cartesian(Pos const& pos_local, double wavelength) const
{
    auto const [r, polar, azimuth] = math::spherical_from_cartesian_pos<std::array<double, 3>>(pos_local);
    return elv_spherical(polar, azimuth, wavelength);
}

double Radiator::calc_directivity_from_spherical(double polar, double azimuth, double wavelength, setup::SimParams const& sim_params) const
{ return math::square(math::norm(elv_spherical(polar, azimuth, wavelength))) / calc_ms_elv(elv_spherical, wavelength, sim_params); }

double Radiator::calc_directivity_from_cartesian(Pos const& pos_local, double wavelength, setup::SimParams const& sim_params) const
{
    auto const [r, polar, azimuth] = math::spherical_from_cartesian_pos<std::array<double, 3>>(pos_local);
    return calc_directivity_from_spherical(polar, azimuth, wavelength, sim_params);
}
