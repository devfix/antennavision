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
            { return elv_spherical(polar, azimuth, wavelength, dipole_length); },[dipole_length](double wavelength) -> msel_t::result_type { return msel(wavelength, dipole_length); }};
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
        (polar_edges(polar_edges.rSlice(), nc::Slice(0, static_cast<std::int32_t>(n_polar))) + polar_edges(polar_edges.rSlice(), nc::Slice(1, static_cast<std::int32_t>(n_polar) + 1))) /
        2.0;
    auto const azimuth_mids =
        (azimuth_edges(azimuth_edges.rSlice(), nc::Slice(0, static_cast<std::int32_t>(n_azimuth))) + azimuth_edges(azimuth_edges.rSlice(), nc::Slice(1, static_cast<std::int32_t>(n_azimuth) + 1))) / 2.0;
    auto const [polar_grid, azimuth_grid] = nc::meshgrid(polar_mids, azimuth_mids);

    NdArray squared_norms(polar_grid.shape());
    std::ranges::transform(polar_grid, azimuth_grid, squared_norms.begin(), [&elv_spherical, wavelength](double const polar, double const azimuth) -> double
                           { return math::square(std::real(nc::norm(elv_spherical(polar, azimuth, wavelength)).item())); });

    // Reshape squared_norms back to match the grid shape (num_azimuth x num_polar)
    squared_norms = squared_norms.reshape(polar_grid.shape());

    // 7. Compute the integrand: || l_e ||^2 * sin(polar)
    auto integrand = squared_norms * nc::sin(polar_grid);

    double integral = nc::sum(integrand).item() * d_polar * d_azimuth;
    return integral / (4.0 * pi);
}

nc::NdArray<complex_t> Radiator::calc_elv_spherical_from_cartesian(Vec3 const &pos_local, double const wavelength) const
{
    auto const [r, polar, azimuth] = math::spherical_from_cartesian(pos_local);
    return elv_spherical(polar, azimuth, wavelength);
}

complex_t Radiator::calc_path(std::size_t idx_input, std::size_t idx_output) { throw std::runtime_error("calc_path() should not be called on a radiator"); }

double Radiator::calc_radiation_resistance(std::size_t n_polar, std::size_t n_azimuth) const {}

double Radiator::calc_directivity(double polar, double azimuth, double const wavelength, std::size_t n_polar, std::size_t n_azimuth) const
{
    double num = math::norm(elv_spherical(polar, azimuth, wavelength));
    num *= num * 4 * pi;
    auto polar_edges = nc::linspace(0.0, pi, n_polar + 1);
    auto azimuth_edges = nc::linspace(0.0, 2.0 * pi, n_azimuth + 1);
    auto d_polar = pi / static_cast<double>(n_polar);
    auto d_azimuth = (2.0 * pi) / static_cast<double>(n_azimuth);
    auto polar_mids =
        (polar_edges(polar_edges.rSlice(), nc::Slice(0, static_cast<std::int32_t>(n_polar))) + polar_edges(polar_edges.rSlice(), nc::Slice(1, static_cast<std::int32_t>(n_polar) + 1))) /
        2.0;
    auto azimuth_mids =
        (azimuth_edges(azimuth_edges.rSlice(), nc::Slice(0, static_cast<std::int32_t>(n_azimuth))) + azimuth_edges(azimuth_edges.rSlice(), nc::Slice(1, static_cast<std::int32_t>(n_azimuth) + 1))) / 2.0;

    // 4. Create a 2D meshgrid of the coordinates
    auto [polar_grid, azimuth_grid] = nc::meshgrid(polar_mids, azimuth_mids);

    // 5. Evaluate the 3D vector function over the flattened grid
    // polar_grid and azimuth_grid from meshgrid are 2D arrays (num_azimuth x num_polar)
    NdArray norm_result(polar_grid.shape());

    // 2. Use std::transform to pair elements up and apply the lambda
    std::ranges::transform(polar_grid, azimuth_grid, norm_result.begin(),
                           [this, wavelength](double const polar, double const azimuth) { return math::norm(elv_spherical(polar, azimuth, wavelength)); });

    // 6. Compute the squared norm || l_e ||^2 along the coordinate axis
    // vectors_flattened has shape (num_points, 3). We square and sum across rows (Axis::COL).
    auto squared_norms = nc::square(norm_result);

    // Reshape squared_norms back to match the grid shape (num_azimuth x num_polar)
    squared_norms = squared_norms.reshape(polar_grid.shape());

    // 7. Compute the integrand: || l_e ||^2 * sin(polar)
    auto integrand = squared_norms * nc::sin(polar_grid);

    // 8. Perform the double integration by summing all elements and multiplying by differentials
    double total_sum = nc::sum(integrand).item();
    double integral_result = total_sum * d_polar * d_azimuth;
    double result = num / integral_result;
    return result;
}

double Radiator::calc_directivity(Vec3 const &pos_local, double wavelength) const
{
    auto const [r, polar, azimuth] = math::spherical_from_cartesian(pos_local);
    return calc_directivity(polar, azimuth, wavelength);
}

complex_t Radiator::calc_voltage_gain(Radiator const &radiator, double const wavelength, std::size_t n_polar, std::size_t n_azimuth) const
{
    double const r = (origin.global_from_local_pos(POS_ZERO) - radiator.origin.global_from_local_pos(POS_ZERO)).norm();
    if (r < wavelength / 10) { std::println("Warning: Radiator {} is very close to radiator {}, distance: {} m ({} λ)", id, radiator.id, r, r / wavelength); }
    auto const [r1, polar1, azimuth1] = math::spherical_from_cartesian(origin.localize(radiator.origin));
    auto const omega1 = math::omega(polar1, azimuth1);
    auto const [r2, polar2, azimuth2] = math::spherical_from_cartesian(radiator.origin.localize(origin));
    auto const omega2 = math::omega(polar2, azimuth2);
    auto const l1_spherial = calc_elv_spherical_from_cartesian(origin.localize(radiator.origin), wavelength);
    auto const l2_spherial = radiator.calc_elv_spherical_from_cartesian(radiator.origin.localize(origin), wavelength);

    auto const l1_rot = nc::dot(omega1, l1_spherial);
    auto const l2_rot = nc::dot(omega2, l2_spherial);

    auto a = Vec3(nc::real(l1_rot));
    auto b = Vec3(nc::real(l2_rot));

    auto const l1 = origin.global_from_local_vec(l1_rot);
    auto const l2 = radiator.origin.global_from_local_vec(l2_rot);

    auto c = Vec3(nc::real(l1));
    auto d = Vec3(nc::real(l2));

    auto const g = l1.dot(l2).item();

    // auto const g = nc::dot(l1_rot, l2_rot).item();

    auto const num = -2.0 * complex_t(0.0, 1.0) * wavelength * g;
    auto polar_edges = nc::linspace(0.0, pi, n_polar + 1);
    auto azimuth_edges = nc::linspace(0.0, 2.0 * pi, n_azimuth + 1);
    auto d_polar = pi / static_cast<double>(n_polar);
    auto d_azimuth = (2.0 * pi) / static_cast<double>(n_azimuth);
    auto polar_mids =
        (polar_edges(polar_edges.rSlice(), nc::Slice(0, static_cast<std::int32_t>(n_polar))) + polar_edges(polar_edges.rSlice(), nc::Slice(1, static_cast<std::int32_t>(n_polar) + 1))) /
        2.0;
    auto azimuth_mids =
        (azimuth_edges(azimuth_edges.rSlice(), nc::Slice(0, static_cast<std::int32_t>(n_azimuth))) + azimuth_edges(azimuth_edges.rSlice(), nc::Slice(1, static_cast<std::int32_t>(n_azimuth) + 1))) / 2.0;

    // 4. Create a 2D meshgrid of the coordinates
    auto [polar_grid, azimuth_grid] = nc::meshgrid(polar_mids, azimuth_mids);

    // 5. Evaluate the 3D vector function over the flattened grid
    // polar_grid and azimuth_grid from meshgrid are 2D arrays (num_azimuth x num_polar)
    NdArray norm_result(polar_grid.shape());

    // 2. Use std::transform to pair elements up and apply the lambda
    std::ranges::transform(polar_grid, azimuth_grid, norm_result.begin(),
                           [this, wavelength](double const polar, double const azimuth) { return math::norm(elv_spherical(polar, azimuth, wavelength)); });

    // 6. Compute the squared norm || l_e ||^2 along the coordinate axis
    // vectors_flattened has shape (num_points, 3). We square and sum across rows (Axis::COL).
    auto squared_norms = nc::square(norm_result);

    // Reshape squared_norms back to match the grid shape (num_azimuth x num_polar)
    squared_norms = squared_norms.reshape(polar_grid.shape());

    // 7. Compute the integrand: || l_e ||^2 * sin(polar)
    auto integrand = squared_norms * nc::sin(polar_grid);

    // 8. Perform the double integration by summing all elements and multiplying by differentials
    double total_sum = nc::sum(integrand).item();
    double integral_result = total_sum * d_polar * d_azimuth;
    complex_t result = num / integral_result;

    complex_t const phase_term = std::exp(complex_t(0.0, -2.0 * pi * r / wavelength)) / r;

    return result * phase_term;
}

double Radiator::calc_power_gain(Radiator const &radiator, double const wavelength) const
{
    // double const gain_self = calc_directivity(origin.localize(radiator.origin));
    // double const gain_other = radiator.calc_directivity(radiator.origin.localize(origin));
    // double const r = (origin.global_from_local(POS_ZERO) - radiator.origin.global_from_local(POS_ZERO)).norm();
    // return gain_self * gain_other * std::pow(lambda / (4.0 * PI * r), 2.0);
    return math::square(std::abs(calc_voltage_gain(radiator, wavelength))) / 4.0;
}

// complex_t Radiator::calc_radiation_gain(position_t const &pos, double freq) const
// {
//     return get_relative_gain(get_relative_position(pos), freq);
// }
