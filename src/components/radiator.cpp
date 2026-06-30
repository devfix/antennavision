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
#include "NumCpp/Functions/multiply.hpp"
#include "NumCpp/Functions/real.hpp"
#include "math.hpp"
#include "print.hpp"

Radiator Radiator::HertzianDipole::create(std::string_view id, Reference const &origin) { return {id, origin, elv_spherical, msel}; }

Radiator::elv_spherical_t::result_type Radiator::HertzianDipole::elv_spherical(double const polar, double, double) { return math::vec<complex_t>(0.0, -HERTZIAN_DIPOLE_LENGTH * std::sin(polar), 0.0); }

Radiator::msel_t::result_type Radiator::HertzianDipole::msel(double) { return 2.0 / 3.0 * math::square(HERTZIAN_DIPOLE_LENGTH); }

Radiator Radiator::StandingWaveDipole::create(std::string_view id, Reference const &origin, double const dipole_length)
{
    return {id, origin, [dipole_length](double const polar, double const azimuth, double const wavelength) -> elv_spherical_t::result_type
            { return elv_spherical(polar, azimuth, wavelength, dipole_length); }, [dipole_length](double wavelength) -> msel_t::result_type { return msel(wavelength, dipole_length); }};
}

Radiator::elv_spherical_t::result_type Radiator::StandingWaveDipole::elv_spherical(double polar, double azimuth, double wavelength, double dipole_length)
{
    double const x = pi * dipole_length / wavelength;
    complex_t polar_comp = -wavelength / (pi * std::sin(polar)) * (std::cos(x * std::cos(polar)) - cos(x));
    return {0.0, polar_comp, 0.0};
}

Radiator::msel_t::result_type Radiator::StandingWaveDipole::msel(double wavelength, double const dipole_length)
{
    double const x = pi * dipole_length / wavelength;
    return 0.5 * math::square(wavelength / pi) * math::q_function(x);
}

Radiator::Radiator(std::string_view const id, Reference const &origin, elv_spherical_t elv_spherical, msel_t msel) :
    Component(id, 1, 0), origin(origin), elv_spherical(std::move(elv_spherical)), msel(std::move(msel))
{}

nc::NdArray<complex_t> Radiator::get_elv_spherical_standing_wave(double const dipole_length, double const wavelength, double const polar)
{
    double const x = pi * dipole_length / wavelength;
    complex_t polar_comp = -wavelength / (pi * std::sin(polar)) * (std::cos(x * std::cos(polar)) - cos(x));
    return {0.0, polar_comp, 0.0};
}

double Radiator::calc_mean_squared_effective_length(elv_spherical_t const &elv_spherical, double wavelength, std::size_t n_polar, std::size_t n_azimuth)
{
    auto const polar_edges = nc::linspace(0.0, pi, n_polar + 1);
    auto const azimuth_edges = nc::linspace(0.0, 2.0 * pi, n_azimuth + 1);
    auto const d_polar = pi / static_cast<double>(n_polar);
    auto const d_azimuth = 2.0 * pi / static_cast<double>(n_azimuth);

    auto const polar_mids =
        (polar_edges(polar_edges.rSlice(), nc::Slice(0, static_cast<std::int32_t>(n_polar))) + polar_edges(polar_edges.rSlice(), nc::Slice(1, static_cast<std::int32_t>(n_polar) + 1))) / 2.0;
    auto const azimuth_mids =
        (azimuth_edges(azimuth_edges.rSlice(), nc::Slice(0, static_cast<std::int32_t>(n_azimuth))) + azimuth_edges(azimuth_edges.rSlice(), nc::Slice(1, static_cast<std::int32_t>(n_azimuth) + 1))) /
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

nc::NdArray<complex_t> Radiator::calc_elv_spherical_from_cartesian(Vec3 const &pos_local, double const wavelength) const
{
    auto const [r, polar, azimuth] = math::spherical_from_cartesian(pos_local);
    return elv_spherical(polar, azimuth, wavelength);
}

complex_t Radiator::calc_path(std::size_t idx_input, std::size_t idx_output) { throw std::runtime_error("calc_path() should not be called on a radiator"); }

double Radiator::calc_radiation_resistance(std::size_t n_polar, std::size_t n_azimuth) const {}

double Radiator::calc_directivity_from_spherical(double polar, double azimuth, double const wavelength, std::size_t n_polar, std::size_t n_azimuth) const
{
    return math::square(math::norm(elv_spherical(polar, azimuth, wavelength))) / calc_mean_squared_effective_length(elv_spherical, wavelength, n_polar, n_azimuth);
}

double Radiator::calc_directivity_from_cartesian(Vec3 const &pos_local, double const wavelength) const
{
    auto const [r, polar, azimuth] = math::spherical_from_cartesian(pos_local);
    return calc_directivity_from_spherical(polar, azimuth, wavelength);
}

complex_t Radiator::calc_voltage_gain(Radiator const &radiator_tx, Radiator const &radiator_rx, double const wavelength, std::size_t const n_polar, std::size_t const n_azimuth)
{
    double const r = (radiator_tx.origin.global_from_local_pos(POS_ZERO) - radiator_rx.origin.global_from_local_pos(POS_ZERO)).norm();
    if (r < wavelength / 10) { std::println("Warning: Radiator {} is very close to radiator {}, distance: {} m ({} λ)", radiator_tx.id, radiator_rx.id, r, r / wavelength); }

    auto const pos_local_tx = radiator_tx.origin.localize(radiator_rx.origin); // position of rx radiator in tx coordinate
    auto const pos_local_rx = radiator_rx.origin.localize(radiator_tx.origin); // position of tx radiator in rx coordinate
    auto const rot_mat_tx = math::get_rot_mat_from_cartesian(pos_local_tx);
    auto const rot_mat_rx = math::get_rot_mat_from_cartesian(pos_local_rx);
    auto const elv_spherical_tx = radiator_tx.calc_elv_spherical_from_cartesian(pos_local_tx, wavelength);
    auto const elv_spherical_rx = radiator_rx.calc_elv_spherical_from_cartesian(pos_local_rx, wavelength);
    auto const elv_cartesian_tx = nc::dot(rot_mat_tx, elv_spherical_tx);
    auto const elv_cartesian_rx = nc::dot(rot_mat_rx, elv_spherical_rx);
    auto const elv_global_tx = radiator_tx.origin.global_from_local_vec(elv_cartesian_tx);
    auto const elv_global_rx = radiator_rx.origin.global_from_local_vec(elv_cartesian_rx);
    auto const g = elv_global_tx.dot(elv_global_rx).item();
    auto const propagation = std::exp(-j * 2.0 * pi * r / wavelength) * wavelength / (4.0 * pi * r);
    auto const leffmean_tx = calc_mean_squared_effective_length(radiator_tx.elv_spherical, wavelength, n_polar, n_azimuth);
    auto const leffmean_rx = calc_mean_squared_effective_length(radiator_rx.elv_spherical, wavelength, n_polar, n_azimuth);
    return -j * g / std::sqrt(leffmean_tx * leffmean_rx) * propagation;
}

double Radiator::calc_power_gain(Radiator const &radiator_tx, Radiator const &radiator_rx, double const wavelength)
{
    return math::square(std::abs(calc_voltage_gain(radiator_tx, radiator_rx, wavelength)));
}
