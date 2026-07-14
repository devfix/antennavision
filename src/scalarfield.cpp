//
// Created by core on 04.07.26.
//

#include "scalarfield.hpp"
#include <ansi_color.hpp>
#include <nlopt.hpp>
#include <utility>
#include <print>

template <typename ScalarType>
ScalarField<ScalarType>::ScalarField(std::string_view id, field_t&& field, reset_t&& reset, math::NumParams const& num_params) : id(id), field(std::move(field)), num_params(num_params), reset(std::move(reset)) {}

template <typename ScalarType>
ScalarField<ScalarType>::~ScalarField()
{
    if (reset) { reset(); }
}

template <typename ScalarType>
std::pair<pos_t, double> ScalarField<ScalarType>::argmax_line_abs(pos_t const& pos_a, pos_t const& pos_b) const
{
    pos_t const delta = pos_b - pos_a;
    math::OptParams const params{[this, &pos_a, &delta](double const x) -> double { return -std::abs(field(pos_a + x * delta, num_params.wavelength)); }, 0.0, 1.0, num_params};
    auto [x_min, f_min] = math::f_min(params);
    pos_t pos_max = pos_a + x_min * delta;
    return {pos_max, std::abs(field(pos_max, num_params.wavelength))};
}

template <typename ScalarType>
std::pair<pos_t, double> ScalarField<ScalarType>::argmax_circle_abs(math::Circle const& circle, double const angle) const
{
    math::OptParams const params{[&](double const x) -> double
                                 {
                                     pos_t const pos = circle.center + circle.radius * (std::cos(x) * circle.v1 + std::sin(x) * circle.v2);
                                     return -std::abs(field(pos, num_params.wavelength));
                                 },
                                 0.0, angle, num_params};
    auto [angle_min, f_min] = math::f_min(params);
    pos_t pos_max = circle.center + circle.radius * (std::cos(angle_min) * circle.v1 + std::sin(angle_min) * circle.v2);
    return {pos_max, std::abs(field(pos_max, num_params.wavelength))};
}

template <typename ScalarType>
std::pair<pos_t, double> ScalarField<ScalarType>::calc_beamwidth(math::Circle const& circle, double const ratio) const
{
    math::Circle circle_rot(circle);
    circle_rot.rotate_base(-pi / 4.0);
    auto [pos_beam, intensity] = argmax_circle_abs(circle_rot, pi / 2.0);

    auto circle_hpbw = math::get_circle(circle.center, circle.normal, circle.radius, pos_beam - circle.center);
    math::OptParams const params{[&](double const x) -> double
                                 {
                                     pos_t const pos = circle_hpbw.center + circle_hpbw.radius * (std::cos(x) * circle_hpbw.v1 + std::sin(x) * circle_hpbw.v2);
                                     return math::square(std::abs(field(pos, num_params.wavelength)) - ratio * intensity);
                                 },
                                 0.0, pi / 4.0, num_params};
    auto [angle1, eps1] = math::f_min(params);

    circle_hpbw.rotate_base(-pi / 4.0);
    auto [angle2_inv, eps2] = math::f_min(params);
    double angle2 = pi/4.0 - angle2_inv;

    return {pos_beam, angle1 + angle2};
}

template <typename ScalarType>
std::pair<PositionArray, std::variant<RealArray, ComplexArray>> ScalarField<ScalarType>::eval_line(pos_t const& pos_start, pos_t const& pos_end) const
{
    PositionArray positions(num_params.n_linear1, 1);
    nc::NdArray<ScalarType> values(num_params.n_linear1, 1);
    for (ComplexArray::index_type k = 0; k < num_params.n_linear1; k++)
    {
        double const f = static_cast<double>(k) / static_cast<double>(num_params.n_linear1 - 1);
        pos_t const pos = pos_start + f * (pos_end - pos_start);
        positions(k, 0) = pos;
        values(k, 0) = field(pos, num_params.wavelength);
    }
    return {positions, values};
}

template <typename ScalarType>
std::pair<PositionArray, std::variant<RealArray, ComplexArray>> ScalarField<ScalarType>::eval_plane(math::Rectangle const& rectangle) const
{
    PositionArray positions(num_params.n_linear2, num_params.n_linear1);
    nc::NdArray<ScalarType> values(num_params.n_linear2, num_params.n_linear1);
    for (ComplexArray::index_type k_ax2 = 0; k_ax2 < num_params.n_linear2; k_ax2++)
    {
        std::print("k_ax2 = {:04d} / {:04d}\n", k_ax2, num_params.n_linear2);
        double const f_ax2 = static_cast<double>(k_ax2) / static_cast<double>(num_params.n_linear2 - 1);
        for (RealArray::index_type k_ax1 = 0; k_ax1 < num_params.n_linear1; k_ax1++)
        {
            double const f_ax1 = static_cast<double>(k_ax1) / static_cast<double>(num_params.n_linear1 - 1);
            auto const pos = rectangle.center + (f_ax1 - 0.5) * rectangle.width * rectangle.v1 + (f_ax2 - 0.5) * rectangle.height * rectangle.v2;
            positions(k_ax2, k_ax1) = pos;
            values(k_ax2, k_ax1) = field(pos, num_params.wavelength);
        }
    }
    return {positions, values};
}

// =========================================================================
// Explicit Instantiation
// =========================================================================
// This forces the compiler to generate the machine code for these two types
// inside scalarfield.o so that the linker can find them.
template class ScalarField<double>;
template class ScalarField<complex_t>;
