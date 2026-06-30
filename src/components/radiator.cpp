//
// Created by Tristan Krause on 2026-04-28.
//

#include "components/radiator.hpp"
#include <NumCpp/Functions/linspace.hpp>
#include <NumCpp/Functions/meshgrid.hpp>
#include <NumCpp/Functions/sin.hpp>
#include <NumCpp/Functions/sum.hpp>
#include <string>
#include <utility>
#include "factory/get.hpp"
#include "factory/make.hpp"
#include "math.hpp"
#include "print.hpp"

Radiator Radiator::HertzianDipole::create(std::string_view id, Reference & origin) { return {id, origin, elv_spherical, ms_elv}; }

Radiator::elv_spherical_t::result_type Radiator::HertzianDipole::elv_spherical(double const polar, double, double) { return math::vec<complex_t>(0.0, -HERTZIAN_DIPOLE_LENGTH * std::sin(polar), 0.0); }

Radiator::ms_elv_t::result_type Radiator::HertzianDipole::ms_elv(double) { return 2.0 / 3.0 * math::square(HERTZIAN_DIPOLE_LENGTH); }

Radiator Radiator::StandingWaveDipole::create(std::string_view id, Reference & origin, double const dipole_length)
{
    return {id, origin, [dipole_length](double const polar, double const azimuth, double const wavelength) -> elv_spherical_t::result_type
            { return elv_spherical(polar, azimuth, wavelength, dipole_length); }, [dipole_length](double wavelength) -> ms_elv_t::result_type { return ms_elv(wavelength, dipole_length); }};
}

Radiator::elv_spherical_t::result_type Radiator::StandingWaveDipole::elv_spherical(double const polar, double const azimuth, double const wavelength, double const dipole_length)
{
    double const x = pi * dipole_length / wavelength;
    complex_t polar_comp = -wavelength / (pi * std::sin(polar)) * (std::cos(x * std::cos(polar)) - cos(x));
    return {0.0, polar_comp, 0.0};
}

Radiator::ms_elv_t::result_type Radiator::StandingWaveDipole::ms_elv(double const wavelength, double const dipole_length)
{
    double const x = pi * dipole_length / wavelength;
    return 0.5 * math::square(wavelength / pi) * math::q_function(x);
}

Radiator::Radiator(std::string_view const id, Reference & origin, elv_spherical_t elv_spherical, ms_elv_t ms_elv) :
    Component(id, 1, 0), origin(origin), elv_spherical(std::move(elv_spherical)), mean_squared_elv(std::move(ms_elv))
{}

vec_t Radiator::get_elv_spherical_standing_wave(double const dipole_length, double const wavelength, double const polar)
{
    double const x = pi * dipole_length / wavelength;
    complex_t polar_comp = -wavelength / (pi * std::sin(polar)) * (std::cos(x * std::cos(polar)) - cos(x));
    return {0.0, polar_comp, 0.0};
}

double Radiator::calc_mean_squared_effective_length(elv_spherical_t const& elv_spherical, double wavelength, math::NumParams const& num_params)
{
    auto const polar_edges = nc::linspace(0.0, pi, num_params.n_polar + 1);
    auto const azimuth_edges = nc::linspace(0.0, 2.0 * pi, num_params.n_azimuth + 1);
    auto const d_polar = pi / static_cast<double>(num_params.n_polar);
    auto const d_azimuth = 2.0 * pi / static_cast<double>(num_params.n_azimuth);

    auto const polar_mids = (polar_edges(polar_edges.rSlice(), nc::Slice(0, static_cast<std::int32_t>(num_params.n_polar))) +
                             polar_edges(polar_edges.rSlice(), nc::Slice(1, static_cast<std::int32_t>(num_params.n_polar) + 1))) /
        2.0;
    auto const azimuth_mids = (azimuth_edges(azimuth_edges.rSlice(), nc::Slice(0, static_cast<std::int32_t>(num_params.n_azimuth))) +
                               azimuth_edges(azimuth_edges.rSlice(), nc::Slice(1, static_cast<std::int32_t>(num_params.n_azimuth) + 1))) /
        2.0;
    auto const [polar_grid, azimuth_grid] = nc::meshgrid(polar_mids, azimuth_mids);

    NdArray squared_norms(polar_grid.shape());
    std::ranges::transform(polar_grid, azimuth_grid, squared_norms.begin(),
                           [&elv_spherical, wavelength](double const polar, double const azimuth) -> double { return math::square(math::norm(elv_spherical(polar, azimuth, wavelength))); });

    // Reshape squared_norms back to match the grid shape (num_azimuth x num_polar)
    squared_norms = squared_norms.reshape(polar_grid.shape());

    // 7. Compute the integrand: || l_e ||^2 * sin(polar)
    auto const integrand = squared_norms * nc::sin(polar_grid);

    double const integral = nc::sum(integrand).item() * d_polar * d_azimuth;
    return integral / (4.0 * pi);
}

vec_t Radiator::get_elv_spherical_from_cartesian(pos_t const& pos_local, double const wavelength) const
{
    auto const [r, polar, azimuth] = math::spherical_from_cartesian(pos_local);
    return elv_spherical(polar, azimuth, wavelength);
}

double Radiator::calc_directivity_from_spherical(double polar, double azimuth, double const wavelength, math::NumParams const& num_params) const
{ return math::square(math::norm(elv_spherical(polar, azimuth, wavelength))) / calc_mean_squared_effective_length(elv_spherical, wavelength, num_params); }

double Radiator::calc_directivity_from_cartesian(pos_t const& pos_local, double const wavelength, math::NumParams const& num_params) const
{
    auto const [r, polar, azimuth] = math::spherical_from_cartesian(pos_local);
    return calc_directivity_from_spherical(polar, azimuth, wavelength, num_params);
}

complex_t Radiator::calc_path(std::size_t idx_input, std::size_t idx_output) { throw std::runtime_error("calc_path() should not be called on a radiator"); }
