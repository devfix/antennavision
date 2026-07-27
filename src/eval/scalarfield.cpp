//
// Created by Tristan Krause on 2026-07-23.
//

#include "eval/scalarfield.hpp"
#include "eval/voltagefield.hpp"

namespace
{
    template <typename T>
    [[nodiscard]] constexpr double dbl(T val) noexcept
    { return static_cast<double>(val); }
} // namespace

template <typename Derived, typename ScalarT>
nc::NdArray<ScalarT> ScalarField<Derived, ScalarT>::eval(Vec3Array const& positions, double wavelength) const
{

    nc::NdArray<ScalarT> values(positions.shape());
    std::ranges::transform(positions, values.begin(), [this, &wavelength](Pos const& pos) { return field(pos, wavelength); });
    return values;
}

template <typename Derived, typename ScalarT>
[[nodiscard]] std::vector<nc::NdArray<ScalarT>> ScalarField<Derived, ScalarT>::eval_sweep(Vec3Array const& positions, sweep::Sweep const& sweep) const
{
    std::vector<nc::NdArray<ScalarT>> data;
    data.reserve(sweep::get_size(sweep));
    std::ranges::transform(sweep::get_values(sweep),
        std::back_insert_iterator(data),
        [this, &positions](double wavelength) { return eval(positions, wavelength); });
    return data;
}

template <typename Derived, typename ScalarT>
ScalarField<Derived, ScalarT>::EvalResult ScalarField<Derived, ScalarT>::eval_geometry(geometry::Geometry const& geo, double wavelength) const
{
    Vec3Array const positions = geometry::get_positions(geo, num_params.n_linear1, num_params.n_linear2);
    return {positions, eval(positions, wavelength)};
}

template <typename Derived, typename ScalarT>
ScalarField<Derived, ScalarT>::EvalSweepResult ScalarField<Derived, ScalarT>::eval_geometry_sweep(geometry::Geometry const& geo,
    sweep::Sweep const& sweep) const
{
    Vec3Array const positions = geometry::get_positions(geo, num_params.n_linear1, num_params.n_linear2);
    return {positions, eval_sweep(positions, sweep)};
}

template <typename Derived, typename ScalarT>
ScalarField<Derived, ScalarT>::ArgMaxResult ScalarField<Derived, ScalarT>::argmax_curve_abs(geometry::Curve const& curve, double wavelength) const
{
    math::OptParams const params{
        [this, &curve, &wavelength](double const t) -> double { return -std::abs(field(geometry::curve::get_pos_at(curve, t), wavelength)); },
        0.0,
        1.0,
        num_params //
    };
    auto opt_result = math::f_min(params);
    return {opt_result.t_min, geometry::curve::get_pos_at(curve, opt_result.t_min), -opt_result.f_min};
}

template <typename Derived, typename ScalarT>
ScalarField<Derived, ScalarT>::CurvePeakSpan
ScalarField<Derived, ScalarT>::find_curve_peak_and_cutoffs(geometry::Curve const& curve, double wavelength, double ratio) const
{
    std::string const& curve_id = std::visit([](auto const& shape) -> std::string const& { return shape.id(); }, curve);

    // Important: math::scan_f_min and math::f_min find a local minimum. We want to find a maximum we flip the sign in the objective function.
    // The data in opt_peak, opt_left, and opt_right is therefor inverted as well!

    // step 1: find the maximum
    math::OptParams const params_max{
        [this, &curve, &wavelength](double const t) -> double { return -std::abs(field(geometry::curve::get_pos_at(curve, t), wavelength)); },
        0.0,
        1.0,
        num_params //
    };
    auto const opt_peak = math::scan_f_min(params_max);
    if (opt_peak.k_min < 2 or opt_peak.k_min > opt_peak.scan_t.size() - 3)
    {
        throw SimulationError("Curve maximum is at begin or end of curve '{}'", curve_id);
    }
    double const f_peak = -opt_peak.opt.f_min;
    double const thres = ratio * f_peak;

    // step 2: find left cutoff search bound
    std::size_t k_lower = opt_peak.k_min;
    for (; k_lower > 0 and -opt_peak.scan_f[k_lower] >= thres; --k_lower) {}
    if (k_lower == 0) { throw SimulationError("Could not find left cutoff bound of curve '{}'", curve_id); }

    // step 3: find right cutoff search bound
    std::size_t k_upper = opt_peak.k_min;
    for (; k_upper > 0 and -opt_peak.scan_f[k_upper] >= thres; ++k_upper) {}
    if (k_upper == opt_peak.scan_t.size() - 1) { throw SimulationError("Could not find right cutoff bound of curve '{}'", curve_id); }

    // step 4: find left cutoff
    math::OptParams const params_left{
        [this, &curve, &wavelength, thres](double const t) -> double
        { return std::abs(std::abs(field(geometry::curve::get_pos_at(curve, t), wavelength)) - thres); },
        dbl(k_lower) / dbl(opt_peak.scan_t.size() - 1),
        opt_peak.opt.t_min,
        num_params //
    };
    auto const opt_left = math::f_min(params_left);

    // step 5: find right cutoff
    math::OptParams const params_right{
        [this, &curve, &wavelength, thres](double const t) -> double
        { return std::abs(std::abs(field(geometry::curve::get_pos_at(curve, t), wavelength)) - thres); },
        opt_peak.opt.t_min,
        dbl(k_upper) / dbl(opt_peak.scan_t.size() - 1),
        num_params //
    };
    auto const opt_right = math::f_min(params_right);

    return {
        opt_left.t_min,
        opt_peak.opt.t_min,
        opt_right.t_min,
        geometry::curve::get_pos_at(curve, opt_left.t_min),
        geometry::curve::get_pos_at(curve, opt_peak.opt.t_min),
        geometry::curve::get_pos_at(curve, opt_right.t_min),
    };
}

template <typename Derived, typename ScalarT>
std::pair<Pos, double> ScalarField<Derived, ScalarT>::calc_beamwidth(geometry::CircleArc const& arc, double wavelength, double ratio) const
{
    auto curve_peak_span = find_curve_peak_and_cutoffs(arc, wavelength, ratio);
    double angle_span = std::abs(arc.angle_at(curve_peak_span.t_left) - arc.angle_at(curve_peak_span.t_right));
    return {curve_peak_span.pos_peak, angle_span};
}

// -----------------------------------------------------------------------------
// EXPLICIT INSTANTIATION
// -----------------------------------------------------------------------------
template struct ScalarField<VoltageField, Complex>;
