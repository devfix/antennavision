//
// Created by Tristan Krause on 2026-07-23.
//

#include "eval/scalarfield.hpp"
#include <future>
#include <print>
#include <thread>
#include "eval/complexscalarmathfield.hpp"
#include "eval/opt.hpp"
#include "eval/rxvoltagefield.hpp"
#include "logging.hpp"
#include "manifest.hpp"
#include "math/functions.hpp"

namespace eval
{
    using antennavision::logging::LogLevel;
    using nc::Vec2;

    namespace
    {
        constexpr std::string_view LOG_NAME = "scalarfield";

#if defined(ANTENNAVISION_MULTI_THREADED) && (ANTENNAVISION_MULTI_THREADED == 1)
        bool constexpr MULTI_THREADED_SUPPORTED = true;
#else
        bool constexpr MULTI_THREADED_SUPPORTED = false;
#endif

        // Helper callables
        struct IdentityOp
        {
            template <typename T>
            constexpr decltype(auto) operator()(T&& val) const noexcept
            { return std::forward<T>(val); }
        };

        struct AbsOp
        {
            template <typename T>
            constexpr auto operator()(T&& val) const
            {
                using std::abs;
                return abs(std::forward<T>(val));
            }
        };

        // 2D cross product of two vectors: a x b = a.x * b.y - a.y * b.x
        constexpr double cross_2d(const nc::Vec2& a, const nc::Vec2& b) noexcept { return a.x * b.y - a.y * b.x; }

        /**
         * @brief Checks if line segment (s0 -> s1) intersects segment (p0 -> p1).
         *
         * @param s0 Start point of first segment
         * @param s1 End point of first segment
         * @param p0 Start point of second segment
         * @param p1 End point of second segment
         * @return std::optional<nc::Vec2> Exact intersection point if they cross, std::nullopt otherwise.
         */
        inline std::optional<nc::Vec2>
        check_segment_intersection(const nc::Vec2& s0, const nc::Vec2& s1, const nc::Vec2& p0, const nc::Vec2& p1, double eps = 1e-12)
        {
            const nc::Vec2 r = {s1.x - s0.x, s1.y - s0.y}; // Direction vector of S
            const nc::Vec2 s = {p1.x - p0.x, p1.y - p0.y}; // Direction vector of P

            const double r_cross_s = cross_2d(r, s);

            // Parallel or collinear segments
            if (std::abs(r_cross_s) < eps) { return std::nullopt; }

            const nc::Vec2 qp = {p0.x - s0.x, p0.y - s0.y}; // Vector from s0 to p0

            // Solve for parametric parameters u and v:
            // s0 + u * r = p0 + v * s
            const double u = cross_2d(qp, s) / r_cross_s;
            const double v = cross_2d(qp, r) / r_cross_s;

            // Both parameters must be in [0, 1] for the segments to intersect
            if (u >= 0.0 && u <= 1.0 && v >= 0.0 && v <= 1.0) { return nc::Vec2{s0.x + u * r.x, s0.y + u * r.y}; }

            return std::nullopt;
        }

        template <typename ContextT>

        Vec2 trace_isoline_iteration(geometry::Surface const& surf, ContextT const& ctx, Vec2 ts, double thres, setup::SimParams const& sim_params)
        {
            using namespace eval::opt;
            using geometry::surface::get_pos_at;
            double const diff_delta = 1e-3;
            double const step_delta = 1e-2;

            antennavision::logging::debug("scalarfield", "ts: ({:.04f},{:.04f}) --> {:.04f}", ts.x, ts.y, ctx.abs(get_pos_at(surf, ts.x, ts.y)));
            Vec2 gradient = ctx.surf_grad(surf, ts, diff_delta).normalize();
            Vec2 tangent = ctx.surf_tang(surf, ts, diff_delta).normalize();
            auto predictor = ts + step_delta * tangent;
            auto pred_high = ts + step_delta * gradient;
            auto pred_low = ts - step_delta * gradient;
            double predicted = ctx.abs(get_pos_at(surf, predictor.x, predictor.y));

            if (predicted < thres)
            {
                SingleOpt::Params const params{
                    .bound_a = 0,
                    .bound_b = 1,
                    .fn = [&surf, &ctx, &predictor, &pred_high, &thres](double const t) -> double
                    {
                        auto ts = predictor.lerp(pred_high, t);
                        return std::abs(ctx.abs(get_pos_at(surf, ts.x, ts.y)) - thres);
                    },
                    .arg_initial = -1, //
                };
                auto const result = SingleOpt::run(params, sim_params);
                return predictor.lerp(pred_high, result.arg_min);
            }
            else
            {
                SingleOpt::Params const params{
                    .bound_a = 0,
                    .bound_b = 1,
                    .fn = [&surf, &ctx, &predictor, &pred_low, &thres](double const t) -> double
                    {
                        auto ts = predictor.lerp(pred_low, t);
                        return std::abs(ctx.abs(get_pos_at(surf, ts.x, ts.y)) - thres);
                    },
                    .arg_initial = -1, //
                };
                auto const result = SingleOpt::run(params, sim_params);
                return predictor.lerp(pred_low, result.arg_min);
            }
        }
    } // namespace

    template <typename OutputT>
    std::pair<std::size_t, std::size_t> result::EvalResult<OutputT>::find_max() const
    {
        // returns an NdArray with the 1D flat index of the maximum
        std::size_t const flat_idx = values.argmax()[0];
        std::size_t const num_cols = values.numCols();

        std::size_t const n_max = flat_idx / num_cols; // Row index (0...n-1)
        std::size_t const m_max = flat_idx % num_cols; // Column index (0...m-1)

        return {n_max, m_max};
    }

    template <typename OutputT>
    std::pair<std::size_t, std::size_t> result::EvalResult<OutputT>::find_min() const
    {
        // returns an NdArray with the 1D flat index of the minimum
        std::size_t const flat_idx = values.argmin()[0];
        std::size_t const num_cols = values.numCols();

        std::size_t const n_min = flat_idx / num_cols; // Row index (0...n-1)
        std::size_t const m_min = flat_idx % num_cols; // Column index (0...m-1)

        return {n_min, m_min};
    }

    template <typename Derived, typename ScalarT>
    nc::NdArray<ScalarT> ScalarField<Derived, ScalarT>::eval(Vec3Array const& positions, double wavelength) const
    { return eval_impl<ScalarT, IdentityOp>(positions, wavelength); }

    template <typename Derived, typename ScalarT>
    nc::NdArray<double> ScalarField<Derived, ScalarT>::eval_abs(Vec3Array const& positions, double wavelength) const
    { return eval_impl<double, AbsOp>(positions, wavelength); }

    template <typename Derived, typename ScalarT>
    [[nodiscard]] std::vector<nc::NdArray<ScalarT>> ScalarField<Derived, ScalarT>::eval_sweep(Vec3Array const& positions, sweep::Sweep const& sweep) const
    { return eval_sweep_impl<ScalarT, IdentityOp>(positions, sweep); }

    template <typename Derived, typename ScalarT>
    std::vector<nc::NdArray<double>> ScalarField<Derived, ScalarT>::eval_sweep_abs(Vec3Array const& positions, sweep::Sweep const& sweep) const
    { return eval_sweep_impl<double, AbsOp>(positions, sweep); }

    template <typename Derived, typename ScalarT>
    result::EvalResult<ScalarT>
    ScalarField<Derived, ScalarT>::eval_geometry(geometry::Geometry const& geo, double wavelength, std::size_t n1, std::size_t n2) const
    {
        Vec3Array const positions = geometry::get_positions(geo, n1, n2);
        return {positions, eval(positions, wavelength)};
    }

    template <typename Derived, typename ScalarT>
    result::EvalResult<double>
    ScalarField<Derived, ScalarT>::eval_geometry_abs(geometry::Geometry const& geo, double wavelength, std::size_t n1, std::size_t n2) const
    {
        Vec3Array const positions = geometry::get_positions(geo, n1, n2);
        return {positions, eval_abs(positions, wavelength)};
    }

    template <typename Derived, typename ScalarT>
    result::EvalSweepResult<ScalarT> ScalarField<Derived, ScalarT>::eval_geometry_sweep( //
        geometry::Geometry const& geo,
        sweep::Sweep const& sweep,
        std::size_t n1,
        std::size_t n2 //
    ) const
    {
        Vec3Array const positions = geometry::get_positions(geo, n1, n2);
        return {positions, eval_sweep(positions, sweep)};
    }

    template <typename Derived, typename ScalarT>
    result::EvalSweepResult<double> ScalarField<Derived, ScalarT>::eval_geometry_sweep_abs( //
        geometry::Geometry const& geo,
        sweep::Sweep const& sweep,
        std::size_t n1,
        std::size_t n2 //
    ) const
    {
        Vec3Array const positions = geometry::get_positions(geo, n1, n2);
        return {positions, eval_sweep_abs(positions, sweep)};
    }

    template <typename Derived, typename ScalarT>
    result::ArgMaxCurveResult ScalarField<Derived, ScalarT>::argmax_curve_abs(geometry::Curve const& curve, double wavelength, std::size_t n) const
    {
        auto const [scan_result, opt_result] = argmax_curve_abs_impl(curve, wavelength, n);
        return {opt_result.arg_min, geometry::curve::get_pos_at(curve, opt_result.arg_min), -opt_result.f_min};
    }

    template <typename Derived, typename ScalarT>
    result::ArgMaxSurfaceResult
    ScalarField<Derived, ScalarT>::argmax_surface_abs(geometry::Surface const& surf, double wavelength, std::size_t n1, std::size_t n2) const
    {
        auto const [scan_result, opt_result] = argmax_surface_abs_impl(surf, wavelength, n1, n2);
        return {
            opt_result.args_min[0],
            opt_result.args_min[1],
            geometry::surface::get_pos_at(surf, opt_result.args_min[0], opt_result.args_min[1]),
            -opt_result.f_min //
        };
    }

    template <typename Derived, typename ScalarT>
    result::CurvePeakSpan
    ScalarField<Derived, ScalarT>::find_curve_peak_and_cutoffs(geometry::Curve const& curve, double wavelength, double ratio, std::size_t n) const
    {
        using namespace eval::opt;
        std::string const& curve_id = geometry::get_id(curve);

        auto const scan_result = eval_geometry_abs(geometry::as_geometry(curve), wavelength, n, 0);
        auto const [k_max, m_max] = scan_result.find_max();
        assert(m_max == 0); // second dimension should always have index 0 for a curve
        if (k_max < 2 or k_max > n - 3) throw SimulationError("Curve maximum is too close to begin or end of curve '{}'", curve_id);

        // Important: SingleOpt find a local minimum. Since we want to find a maximum we flip the sign in the objective function.
        // The f_min values in opt_peak, opt_left, and opt_right are therefor inverted the inverted maxima.

        auto const ctx = make_context(wavelength);

        // step 1: find the maximum
        SingleOpt::Params const params_peak{
            .bound_a = math::nidxm1(k_max, n),
            .bound_b = math::nidxp1(k_max, n),
            .fn = [&curve, &ctx](double const t) -> double { return -std::abs(ctx(geometry::curve::get_pos_at(curve, t))); },
            .arg_initial = math::nidx(k_max, n),
        };
        auto const opt_peak = SingleOpt::run(params_peak, sim_params);
        double const peak = -opt_peak.f_min;
        double const thres = ratio * peak;

        // step 2: find left cutoff search bound
        std::size_t k_lower = k_max;
        for (; k_lower > 0 and scan_result.values(k_lower, 0) >= thres; --k_lower) {}
        if (k_lower == 0) throw SimulationError("Could not find left cutoff bound of curve '{}'", curve_id);

        // step 3: find right cutoff search bound
        std::size_t k_upper = k_max;
        for (; k_upper > 0 and scan_result.values(k_upper, 0) >= thres; ++k_upper) {}
        if (k_upper == n - 1) throw SimulationError("Could not find right cutoff bound of curve '{}'", curve_id);

        // step 4: find left cutoff
        SingleOpt::Params const params_left{
            .bound_a = math::nidx(k_lower, n),
            .bound_b = math::nidxp1(k_lower, n),
            .fn = [&curve, thres, &ctx](double t) -> double { return std::abs(std::abs(ctx(geometry::curve::get_pos_at(curve, t))) - thres); },
            .arg_initial = -1 // will be set to midpoint during initialization of optimization
        };
        auto const opt_left = SingleOpt::run(params_left, sim_params);

        // step 5: find right cutoff
        SingleOpt::Params const params_right{
            .bound_a = math::nidxm1(k_upper, n),
            .bound_b = math::nidx(k_upper, n),
            .fn = [&curve, thres, &ctx](double const t) -> double { return std::abs(std::abs(ctx(geometry::curve::get_pos_at(curve, t))) - thres); },
            .arg_initial = -1 // will be set to midpoint during initialization of optimization
        };
        auto const opt_right = SingleOpt::run(params_right, sim_params);

        return {
            .t_left = opt_left.arg_min,
            .t_peak = opt_peak.arg_min,
            .t_right = opt_right.arg_min,
            .peak = peak,
            .pos_left = geometry::curve::get_pos_at(curve, opt_left.arg_min),
            .pos_peak = geometry::curve::get_pos_at(curve, opt_peak.arg_min),
            .pos_right = geometry::curve::get_pos_at(curve, opt_right.arg_min),
        };
    }

    template <typename Derived, typename ScalarT>
    std::pair<Pos, double> ScalarField<Derived, ScalarT>::calc_beamwidth(geometry::CircleArc const& arc, double wavelength, double ratio, std::size_t n) const
    {
        auto curve_peak_span = find_curve_peak_and_cutoffs(arc, wavelength, ratio, n);
        double angle_span = std::abs(arc.angle_at(curve_peak_span.t_left) - arc.angle_at(curve_peak_span.t_right));
        return {curve_peak_span.pos_peak, angle_span};
    }

    template <typename Derived, typename ScalarT>
    std::vector<result::Isoline>
    ScalarField<Derived, ScalarT>::trace_isolines(geometry::Surface const& surf, double wavelength, double ratio, std::size_t n1, std::size_t n2) const
    {
        using namespace opt;
        using geometry::surface::get_pos_at;

        auto [scan_result, opt_result] = argmax_surface_abs_impl(surf, wavelength, n1, n2);
        auto const thres = ratio * (-opt_result.f_min);

        // zero-out all values below the threshold
        for (auto& val : scan_result.values)
            if (val < thres) val = 0.0;

        auto const ctx = make_context(wavelength);

        auto [t1, t2] = opt_result.args_min;
        SingleOpt::Params const params_init{
            .bound_a = 0,
            .bound_b = t1,
            .fn = [&surf, thres, &ctx, t2](double const t) -> double { return std::abs(std::abs(ctx(get_pos_at(surf, t, t2))) - thres); },
            .arg_initial = -1 // will be set to midpoint during initialization of optimization
        };
        auto const opt_init = SingleOpt::run(params_init, sim_params);
        auto isoline = trace_isoline(surf, wavelength, Vec2(opt_init.arg_min, t2), thres);
        return {isoline};
    }

    template <typename Derived, typename ScalarT>
    template <typename OutputT, typename OutputCast>
    nc::NdArray<OutputT> ScalarField<Derived, ScalarT>::eval_impl(Vec3Array const& positions, double wavelength) const
    {
        nc::NdArray<OutputT> values(positions.shape());
        std::size_t const total_size = positions.size();

        // we only perform multithreaded computation if generally enabled and the number of points is large enough
        std::size_t const num_threads = std::thread::hardware_concurrency();
        bool const multi_threaded = MULTI_THREADED_SUPPORTED and total_size >= num_threads;

        if (multi_threaded)
        {
            // Determine thread count and calculate chunk sizes
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
                        auto const ctx = make_context(wavelength);
                        auto it = positions.begin() + static_cast<std::ptrdiff_t>(start_idx);
                        auto ot = values.begin() + start_idx;

                        constexpr OutputCast cast_op{};
                        std::size_t local_counter = 0;
                        for (std::size_t k = start_idx; k < end_idx; ++k, ++it, ++ot)
                        {
                            *ot = cast_op(ctx(*it));

                            // Batch atomic updates to prevent CPU cache contention
                            if (++local_counter >= n_report)
                            {
                                std::size_t const current_total = processed.fetch_add(local_counter, std::memory_order_relaxed) + local_counter;
                                local_counter = 0;

                                std::lock_guard lock(print_mutex);
                                double const p = static_cast<double>(current_total) / static_cast<double>(total_size);
                                antennavision::logging::log( //
                                    LogLevel::INFO,
                                    LOG_NAME,
                                    "eval @ λ={:.06f}m : {: 5.1f}%",
                                    wavelength,
                                    p //
                                );
                                std::cout << std::flush;
                            }
                        }

                        // Add any remaining remainder from this thread's chunk
                        if (local_counter > 0) { processed.fetch_add(local_counter, std::memory_order_relaxed); }
                    }));
            }

            // Wait for all threads to finish computing their chunks
            for (auto& f : futures) f.get();
        }
        else
        {
            auto const ctx = make_context(wavelength);
            auto it = positions.begin();
            auto ot = values.begin();

            constexpr OutputCast cast_op{};
            for (std::size_t k = 0; it != positions.end(); ++it, ++ot, ++k)
            {
                // Periodic terminal update without atomic/mutex overhead
                if (k % N_BATCH_PROGRESS_REPORT == 0)
                {
                    double const p = static_cast<double>(k) / static_cast<double>(total_size);
                    antennavision::logging::log( //
                        LogLevel::INFO,
                        LOG_NAME,
                        "eval @ λ={:.06f}m : {: 5.1f}%",
                        wavelength,
                        p //
                    );
                    std::cout << std::flush;
                }
                *ot = cast_op(ctx(*it));
            }
        }

        // Print final 100% completion cleanup
        antennavision::logging::info(LOG_NAME, "eval @ λ={:.06f}m : 100%", wavelength);

        return values;
    }

    template <typename Derived, typename ScalarT>
    template <typename OutputT, typename OutputCast>
    std::vector<nc::NdArray<OutputT>> ScalarField<Derived, ScalarT>::eval_sweep_impl(Vec3Array const& positions, sweep::Sweep const& sweep) const
    {
        std::vector<nc::NdArray<OutputT>> data;
        data.reserve(sweep::get_size(sweep));
        std::ranges::transform(sweep::get_values(sweep),
            std::back_insert_iterator(data),
            [this, &positions](double wavelength) { return eval_impl<OutputT, OutputCast>(positions, wavelength); });
        return data;
    }

    template <typename Derived, typename ScalarT>
    std::pair<result::EvalResult<double>, opt::SingleOpt::Result>
    ScalarField<Derived, ScalarT>::argmax_curve_abs_impl(geometry::Curve const& curve, double wavelength, std::size_t n) const
    {
        using namespace eval::opt;

        auto const scan_result = eval_geometry_abs(geometry::as_geometry(curve), wavelength, n, 0);
        auto [k_max, m_max] = scan_result.find_max();
        assert(m_max == 0); // second dimension should always have index 0 for a curve

        auto const ctx = make_context(wavelength);

        SingleOpt::Params const params{
            .bound_a = math::nidxm1(k_max, n),
            .bound_b = math::nidxp1(k_max, n),
            .fn = [&curve, &ctx](double const t) -> double { return -std::abs(ctx(geometry::curve::get_pos_at(curve, t))); },
            .arg_initial = math::nidx(k_max, n),
        };
        auto const opt_result = SingleOpt::run(params, sim_params);
        return {scan_result, opt_result};
    }

    template <typename Derived, typename ScalarT>
    std::pair<result::EvalResult<double>, opt::DualOpt::Result>
    ScalarField<Derived, ScalarT>::argmax_surface_abs_impl(geometry::Surface const& surf, double wavelength, std::size_t n1, std::size_t n2) const
    {
        using namespace eval::opt;

        auto const scan_result = eval_geometry_abs(geometry::as_geometry(surf), wavelength, n1, n2);
        auto const [k1_max, k2_max] = scan_result.find_max();

        auto const ctx = make_context(wavelength);

        DualOpt::Params const params{
            .bounds_a = {math::nidxm1(k1_max, n1), math::nidxm1(k2_max, n2)},
            .bounds_b = {math::nidxp1(k1_max, n1), math::nidxp1(k2_max, n2)},
            .fn = [&surf, &ctx](std::span<double const> ts) -> double { return -std::abs(ctx(geometry::surface::get_pos_at(surf, ts[0], ts[1]))); },
            .args_initial = {math::nidx(k1_max, n1), math::nidx(k2_max, n2)},
        };
        auto const opt_result = DualOpt::run(params, sim_params);
        return {scan_result, opt_result};
    }

    template <typename Derived, typename ScalarT>
    result::Isoline ScalarField<Derived, ScalarT>::trace_isoline(geometry::Surface const& surf, double wavelength, Vec2 ts, double thres) const
    {
        using namespace eval::opt;
        using geometry::surface::get_pos_at;
        auto const ctx = make_context(wavelength);

        result::Isoline isoline;
        isoline.surf_points.push_back(ts);
        while (isoline.surf_points.size() < 50)
        {
            ts = trace_isoline_iteration(surf, ctx, ts, thres, sim_params);
            if (isoline.surf_points.size() >= 3)
            {
                auto intersection = check_segment_intersection(isoline.surf_points[0], isoline.surf_points[1], isoline.surf_points.back(), ts);
                if (intersection)
                {
                    antennavision::logging::debug("scalarfield", "isoline closed!");
                    break;
                }
            }
            isoline.surf_points.push_back(ts);
        }
        asm("nop");
        return isoline;
    }

    // -----------------------------------------------------------------------------
    // EXPLICIT INSTANTIATIONS
    // -----------------------------------------------------------------------------
    template struct ScalarField<RxVoltageField, Complex>;
    template struct ScalarField<ComplexScalarMathField, Complex>;
} // namespace eval
