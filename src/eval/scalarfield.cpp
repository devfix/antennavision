//
// Created by Tristan Krause on 2026-07-23.
//

#include "eval/scalarfield.hpp"
#include <future>
#include <print>
#include <thread>

#include "eval/opt.hpp"
#include "eval/rxvoltagefield.hpp"
#include "manifest.hpp"

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
    std::size_t const total_size = positions.size();

#ifndef ANTENNAVISION_SINGLE_THREADED
    auto const ctx = make_context();
    auto it = positions.begin();
    auto ot = values.begin();

    for (std::size_t k = 0; it != positions.end(); ++it, ++ot, ++k)
    {
        // Periodic terminal update without atomic/mutex overhead
        if (k % N_BATCH_PROGRESS_REPORT == 0)
        {
            double const p = static_cast<double>(k) / static_cast<double>(total_size);
            std::print("\033[2K\rScalarField::eval @ λ={:.04f}m : {: 5.1f}%", wavelength, p * 100.0);
            std::cout << std::flush;
        }
        *ot = ctx(*it, wavelength);
    }
#else
    // Determine thread count and calculate chunk sizes
    std::size_t const num_threads = std::thread::hardware_concurrency();
    std::size_t const chunk_size = (total_size + num_threads - 1) / num_threads;
    std::size_t const n_report = N_BATCH_PROGRESS_REPORT * std::max(num_threads / 2, static_cast<std::size_t>(1));

    // --- PROGRESS TRACKING STATE ---
    std::atomic<std::size_t> processed{0};
    std::mutex print_mutex;

    std::vector<std::future<void>> futures;
    futures.reserve(num_threads);

    // Launch 1 task per chunk
    for (std::size_t t = 0; t < num_threads; ++t)
    {
        std::size_t const start_idx = t * chunk_size;
        if (start_idx >= total_size) break;
        std::size_t const end_idx = std::min(start_idx + chunk_size, total_size);

        futures.push_back(std::async(std::launch::async,
            [this, start_idx, end_idx, &positions, &values, wavelength, &processed, &print_mutex, total_size, n_report]
            {
                auto const ctx = make_context();
                auto it = positions.begin() + static_cast<std::ptrdiff_t>(start_idx);
                auto ot = values.begin() + start_idx;

                std::size_t local_counter = 0;
                for (std::size_t k = start_idx; k < end_idx; ++k, ++it, ++ot)
                {
                    *ot = ctx(*it, wavelength);

                    // Batch atomic updates to prevent CPU cache contention
                    if (++local_counter >= n_report)
                    {
                        std::size_t const current_total = processed.fetch_add(local_counter, std::memory_order_relaxed) + local_counter;
                        local_counter = 0;

                        std::lock_guard lock(print_mutex);
                        double const p = static_cast<double>(current_total) / static_cast<double>(total_size);
                        std::print("\033[2K\rScalarField::eval @ λ={:.04f}m : {: 5.1f}%", wavelength, std::min(100.0, p * 100.0));
                        std::cout << std::flush;
                    }
                }

                // Add any remaining remainder from this thread's chunk
                if (local_counter > 0) { processed.fetch_add(local_counter, std::memory_order_relaxed); }
            }));
    }

    // Wait for all threads to finish computing their chunks
    for (auto& f : futures) f.get();
#endif

    // Print final 100% completion cleanup
    std::println("\033[2K\rScalarField::eval @ λ={:.04f}m : done", wavelength);

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
ScalarField<Derived, ScalarT>::EvalResult
ScalarField<Derived, ScalarT>::eval_geometry(geometry::Geometry const& geo, std::size_t n_dim1, std::size_t n_dim2, double wavelength) const
{
    Vec3Array const positions = geometry::get_positions(geo, n_dim1, n_dim2);
    return {positions, eval(positions, wavelength)};
}

template <typename Derived, typename ScalarT>
ScalarField<Derived, ScalarT>::EvalSweepResult ScalarField<Derived, ScalarT>::eval_geometry_sweep( //
    geometry::Geometry const& geo,
    std::size_t n_dim1,
    std::size_t n_dim2,
    sweep::Sweep const& sweep //
) const
{
    Vec3Array const positions = geometry::get_positions(geo, n_dim1, n_dim2);
    return {positions, eval_sweep(positions, sweep)};
}

template <typename Derived, typename ScalarT>
ScalarField<Derived, ScalarT>::ArgMaxCurveResult ScalarField<Derived, ScalarT>::argmax_curve_abs(geometry::Curve const& curve, double wavelength) const
{
    auto const ctx = make_context();
    math::OptParams const params{
        [&curve, &wavelength, &ctx](double const t) -> double { return -std::abs(ctx(geometry::curve::get_pos_at(curve, t), wavelength)); },
        0.0,
        1.0,
        num_params //
    };
    auto opt_result = math::scan_f_min(params);
    return {opt_result.opt.t_min, geometry::curve::get_pos_at(curve, opt_result.opt.t_min), -opt_result.opt.f_min};
}

template <typename Derived, typename ScalarT>
typename ScalarField<Derived, ScalarT>::ArgMaxSurfaceResult ScalarField<Derived, ScalarT>::argmax_surface_abs(geometry::Surface const& surf,
    double wavelength) const
{


}

template <typename Derived, typename ScalarT>
ScalarField<Derived, ScalarT>::CurvePeakSpan
ScalarField<Derived, ScalarT>::find_curve_peak_and_cutoffs(geometry::Curve const& curve, double wavelength, double ratio) const
{
    using namespace eval::opt;

    std::string const& curve_id = std::visit([](auto const& shape) -> std::string const& { return shape.id(); }, curve);

    // Important: math::scan_f_min and math::f_min find a local minimum. We want to find a maximum we flip the sign in the objective function.
    // The data in opt_peak, opt_left, and opt_right is therefor inverted as well!

    // step 1: find the maximum
    auto const ctx = make_context();
    math::OptParams const params_max{
        [&curve, &wavelength, &ctx](double const t) -> double { return -std::abs(ctx(geometry::curve::get_pos_at(curve, t), wavelength)); },
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
    SingleOpt::Params const params_left{
        .bounds_a = {dbl(k_lower) / dbl(opt_peak.scan_t.size() - 1)},
        .bounds_b = {opt_peak.opt.t_min},
        .fn = [&curve, &wavelength, thres, &ctx](double t) -> double
        { return std::abs(std::abs(ctx(geometry::curve::get_pos_at(curve, t), wavelength)) - thres); }//
    };
    auto const opt_left = SingleOpt::run(params_left, num_params);

    // step 5: find right cutoff
    SingleOpt::Params const params_right{
        .bounds_a =opt_peak.opt.t_min,
        .bounds_b =dbl(k_upper) / dbl(opt_peak.scan_t.size() - 1),
        .fn = [&curve, &wavelength, thres, &ctx](double const t) -> double
        { return std::abs(std::abs(ctx(geometry::curve::get_pos_at(curve, t), wavelength)) - thres); }//
    };
    auto const opt_right = SingleOpt::run(params_left, num_params);

    return {
        opt_left.ts_min,
        opt_peak.opt.t_min,
        opt_right.ts_min,
        geometry::curve::get_pos_at(curve, opt_left.ts_min),
        geometry::curve::get_pos_at(curve, opt_peak.opt.t_min),
        geometry::curve::get_pos_at(curve, opt_right.ts_min),
    };
}

template <typename Derived, typename ScalarT>
std::pair<Pos, double> ScalarField<Derived, ScalarT>::calc_beamwidth(geometry::CircleArc const& arc, double wavelength, double ratio) const
{
    auto curve_peak_span = find_curve_peak_and_cutoffs(arc, wavelength, ratio);
    double angle_span = std::abs(arc.angle_at(curve_peak_span.t_left) - arc.angle_at(curve_peak_span.t_right));
    return {curve_peak_span.pos_peak, angle_span};
}

template <typename Derived, typename ScalarT>
std::vector<typename ScalarField<Derived, ScalarT>::Isoline>
ScalarField<Derived, ScalarT>::trace_isolines(geometry::Geometry const& geo, std::size_t n_dim1, std::size_t n_dim2, double wavelength) const
{
    auto const sampled_area = eval_geometry(geo, n_dim1, n_dim2, wavelength);
}

// -----------------------------------------------------------------------------
// EXPLICIT INSTANTIATIONS
// -----------------------------------------------------------------------------
template struct ScalarField<RxVoltageField, Complex>;
