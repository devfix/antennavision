//
// Created by Tristan Krause on 2026-04-28.
//

#include "components/radiator.hpp"
#include <string>

Radiator::Radiator(std::string_view const id, Reference const &origin) : Component(id, 1, 0), origin(origin), el_theta(std::move(el_theta)), el_phi(std::move(el_phi)) {}

double Radiator::calc_el_norm_polar(double const theta, double const phi) const { return calc_el_polar(theta, phi).norm(); }

complex_t Radiator::calc_path(std::size_t idx_input, std::size_t idx_output) { throw std::runtime_error("calc_path() should not be called on a radiator"); }

double Radiator::calc_directivity(double theta, double phi, std::size_t n_theta, std::size_t n_phi) const
{
    double num = calc_el_norm_polar(theta, phi);
    num *= num * 4 * PI;
    auto theta_edges = nc::linspace(0.0, PI, n_theta + 1);
    auto phi_edges = nc::linspace(0.0, 2.0 * PI, n_phi + 1);
    auto d_theta = PI / n_theta;
    auto d_phi = (2.0 * PI) / n_phi;
    auto theta_mids = (theta_edges(theta_edges.rSlice(), nc::Slice(0, n_theta)) + theta_edges(theta_edges.rSlice(), nc::Slice(1, n_theta + 1))) / 2.0;
    auto phi_mids = (phi_edges(phi_edges.rSlice(), nc::Slice(0, n_phi)) + phi_edges(phi_edges.rSlice(), nc::Slice(1, n_phi + 1))) / 2.0;

    // 4. Create a 2D meshgrid of the coordinates
    auto [theta_grid, phi_grid] = nc::meshgrid(theta_mids, phi_mids);

    // 5. Evaluate the 3D vector function over the flattened grid
    // theta_grid and phi_grid from meshgrid are 2D arrays (num_phi x num_theta)
    NdArray norm_result(theta_grid.shape());

    // 2. Use std::transform to pair elements up and apply the lambda
    std::ranges::transform(theta_grid, phi_grid, norm_result.begin(), [this](double const theta_, double const phi_) { return calc_el_norm_polar(theta_, phi_); });

    // 6. Compute the squared norm || l_e ||^2 along the coordinate axis
    // vectors_flattened has shape (num_points, 3). We square and sum across rows (Axis::COL).
    auto squared_norms = nc::square(norm_result);

    // Reshape squared_norms back to match the grid shape (num_phi x num_theta)
    squared_norms = squared_norms.reshape(theta_grid.shape());

    // 7. Compute the integrand: || l_e ||^2 * sin(theta)
    auto integrand = squared_norms * nc::sin(theta_grid);

    // 8. Perform the double integration by summing all elements and multiplying by differentials
    double total_sum = nc::sum(integrand).item();
    double integral_result = total_sum * d_theta * d_phi;
    double result = num / integral_result;
    return result;
}

// complex_t Radiator::calc_radiation_gain(position_t const &pos, double freq) const
// {
//     return get_relative_gain(get_relative_position(pos), freq);
// }
