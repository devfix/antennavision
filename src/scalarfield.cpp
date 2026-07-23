//
// Created by core on 04.07.26.
//

#include "scalarfield.hpp"
#include <ansi_color.hpp>
#include <nlopt.hpp>
#include <print>
#include <utility>
#include "memory.hpp"

template <typename ScalarType>
ScalarField<ScalarType>::ScalarField(std::string_view id, field_t&& field, reset_t&& reset, math::NumParams const& num_params) :
    id(id), field(std::move(field)), num_params(num_params), reset(std::move(reset))
{}

template <typename ScalarType>
ScalarField<ScalarType>::~ScalarField()
{
    if (reset) { reset(); }
}

template <typename ScalarType>
std::pair<pos_t, double> ScalarField<ScalarType>::argmax_line_abs(pos_t const& pos_a, pos_t const& pos_b) const
{
    pos_t const delta = pos_b - pos_a;
    math::OptParams const params{[this, &pos_a, &delta](double const x) -> double { return -std::abs(field(pos_a + x * delta, num_params.system_wavelength)); }, 0.0,
                                 1.0, num_params};
    auto [x_min, f_min] = math::f_min(params);
    pos_t pos_max = pos_a + x_min * delta;
    return {pos_max, std::abs(field(pos_max, num_params.system_wavelength))};
}

template <typename ScalarType>
std::pair<pos_t, double> ScalarField<ScalarType>::argmax_arc_abs(geometry::CircleArc const& arc) const
{
    math::OptParams const params{[&](double angle) -> double { return -std::abs(field(arc.get_pos(angle), num_params.system_wavelength)); },
                                 -0.5 * arc.angle_span, 0.5 * arc.angle_span, num_params};
    auto [angle_max, f_min] = math::f_min(params);
    pos_t pos_max = arc.get_pos(angle_max);
    return {pos_max, std::abs(field(pos_max, num_params.system_wavelength))};
}

template <typename ScalarType>
std::pair<pos_t, double> ScalarField<ScalarType>::calc_beamwidth(geometry::CircleArc const& arc, double const ratio) const
{
    auto const [pos_beam, intensity] = argmax_arc_abs(arc);
    auto circle_hpbw = geometry::CircleArc("", arc.center, arc.normal, pos_beam - arc.center, POS_ZERO, arc.radius, arc.angle_span).normalized();
    math::OptParams const params{[&](double const angle) -> double
                                 {
                                     pos_t const pos = circle_hpbw.get_pos(angle);
                                     return math::square(std::abs(field(pos, num_params.system_wavelength)) - ratio * intensity);
                                 },
                                 0.0, 0.5 * arc.angle_span, num_params};
    auto [angle1, eps1] = math::f_min(params);

    reconstruct_at(circle_hpbw, circle_hpbw.rotate(-0.5 * arc.angle_span));

    auto [angle2_inv, eps2] = math::f_min(params);
    double angle2 = 0.5 * arc.angle_span - angle2_inv;

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
        values(k, 0) = field(pos, num_params.system_wavelength);
    }
    return {positions, values};
}

template <typename ScalarType>
std::tuple<PositionArray, SurfacePositionArray, std::variant<RealArray, ComplexArray>> ScalarField<ScalarType>::eval_plane(geometry::Rectangle const& rectangle) const
{
    PositionArray positions(num_params.n_linear2, num_params.n_linear1);
    SurfacePositionArray surface_positions(num_params.n_linear2, num_params.n_linear1);
    nc::NdArray<ScalarType> values(num_params.n_linear2, num_params.n_linear1);
    for (ComplexArray::index_type k_ax2 = 0; k_ax2 < num_params.n_linear2; k_ax2++)
    {
        std::print("k_ax2 = {:04d} / {:04d}\n", k_ax2, num_params.n_linear2);
        double const f_ax2 = static_cast<double>(k_ax2) / static_cast<double>(num_params.n_linear2 - 1);
        double const y = (f_ax2 - 0.5) * rectangle.height;
        for (RealArray::index_type k_ax1 = 0; k_ax1 < num_params.n_linear1; k_ax1++)
        {
            double const f_ax1 = static_cast<double>(k_ax1) / static_cast<double>(num_params.n_linear1 - 1);
            double const x = (f_ax1 - 0.5) * rectangle.width;
            auto const pos = rectangle.center + x * rectangle.e1 + y * rectangle.e2;
            positions(k_ax2, k_ax1) = pos;
            surface_positions(k_ax2, k_ax1) = {x, y};
            values(k_ax2, k_ax1) = field(pos, num_params.system_wavelength);
        }
    }
    return {positions, surface_positions, values};
}

template <typename ScalarType>
std::tuple<PositionArray, SurfacePositionArray, std::variant<RealArray, ComplexArray>> ScalarField<ScalarType>::eval_sphere(geometry::SphericalRectangle const& sr) const
{
    nc::NdArray<pos_t> positions(num_params.n_polar, num_params.n_azimuth);
    SurfacePositionArray surface_positions(num_params.n_polar, num_params.n_azimuth);
    nc::NdArray<ScalarType> values(num_params.n_polar, num_params.n_azimuth);
    for (ComplexArray::index_type k_polar = 0; k_polar < num_params.n_polar; k_polar++)
    {
        std::print("k_ax2 = {:04d} / {:04d}\n", k_polar, num_params.n_polar);

        // Normalized coordinate from 0.0 to 1.0 along the polar axis (e2 / axis 2)
        double const f_polar = static_cast<double>(k_polar) / static_cast<double>(num_params.n_polar - 1);

        // Map to polar angle offset: theta in [-polar/2, polar/2]
        double const polar = (f_polar - 0.5) * sr.polar_span;
        double const sin_polar = std::sin(polar);
        double const cos_polar = std::cos(polar);

        for (RealArray::index_type k_azimuth = 0; k_azimuth < num_params.n_azimuth; k_azimuth++)
        {
            // Normalized coordinate from 0.0 to 1.0 along the azimuthal axis (e1 / axis 1)
            double const f_azimuth = static_cast<double>(k_azimuth) / static_cast<double>(num_params.n_azimuth - 1);

            // Map to azimuthal angle offset: phi in [-azimuth/2, azimuth/2]
            double const azimuth = (f_azimuth - 0.5) * sr.azimuth_span;
            double const sin_azimuth = std::sin(azimuth);
            double const cos_azimuth = std::cos(azimuth);

            // Compute local unit vector on the sphere's surface relative to the sphere center
            auto const local_normal = cos_polar * cos_azimuth * sr.normal + cos_polar * sin_azimuth * sr.e1 + sin_polar * sr.e2;

            // Project outward to the sphere's surface
            auto const pos = sr.center + sr.radius * local_normal;

            positions(k_polar, k_azimuth) = pos;
            surface_positions(k_polar, k_azimuth) = {azimuth, polar};
            values(k_polar, k_azimuth) = field(pos, num_params.system_wavelength);
        }
    }

    // std::ranges::transform(positions, values.begin(), [&](pos_t const& pos) { return field(pos, num_params.wavelength);});

    return {positions, surface_positions, values};
}

// =========================================================================
// Explicit Instantiation
// =========================================================================
// This forces the compiler to generate the machine code for these two types
// inside scalarfield.o so that the linker can find them.
template class ScalarField<double>;
template class ScalarField<complex_t>;
