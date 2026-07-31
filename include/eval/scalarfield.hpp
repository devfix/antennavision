//
// Created by Tristan Krause on 2026-07-23.
//

#pragma once

#include <utility>
#include "setup/numparams.hpp"
#include "setup/geometry.hpp"
#include "setup/sweep.hpp"
#include "types/math.hpp"

/**
 * @brief Base class implementing the Curiously Recurring Template Pattern (CRTP).
 * @tparam Derived The derived class extending this CRTP base.
 */
template <typename Derived, typename ScalarT>
struct ScalarField
{
    struct CurvePeakSpan
    {
        double t_left;
        double t_peak;
        double t_right;
        Pos pos_left;
        Pos pos_peak;
        Pos pos_right;
    };

    template<typename OutputT>
    struct EvalResult
    {
        Vec3Array positions;
        nc::NdArray<OutputT> values;

        /// Finds the maximum value and returns its 2D grid indices (n_max, m_max)
        [[nodiscard]] std::pair<std::size_t, std::size_t> find_max() const;

        /// Finds the maximum value and returns its 2D grid indices (n_max, m_max)
        [[nodiscard]] std::pair<std::size_t, std::size_t> find_min() const;
    };

    struct EvalSweepResult
    {
        Vec3Array positions;
        std::vector<nc::NdArray<ScalarT>> data;
    };

    struct ArgMaxCurveResult
    {
        double t{};
        Pos pos;
        double max{};
    };

    struct ArgMaxSurfaceResult
    {
        double t1{};
        double t2{};
        Pos pos;
        double max{};
    };

    struct Isoline
    {
        std::vector<Pos> points;
        double area;
    };

    template <typename DerivedContext>
    struct Context
    {
        [[nodiscard]] Context() = default;

        // CRTP Interface
        [[nodiscard]] ScalarT operator()(Pos const& pos, double wavelength) const { return (*static_cast<DerivedContext const*>(this))(pos, wavelength); }
    };

    /**
     * Evaluates a field for given positions
     * @param positions array of positions in space
     * @param wavelength wave propagation wavelength
     * @return vector of arrays with field values for each position
     */
    [[nodiscard]] nc::NdArray<ScalarT> eval(Vec3Array const& positions, double wavelength) const;
    [[nodiscard]] nc::NdArray<double> eval_abs(Vec3Array const& positions, double wavelength) const;

    /**
     * Evaluates a field for given positions
     * @param positions array of positions in space
     * @param sweep sweep of wave propagation wavelengths
     * @return vector of arrays with field values for each position
     */
    [[nodiscard]] std::vector<nc::NdArray<ScalarT>> eval_sweep(Vec3Array const& positions, sweep::Sweep const& sweep) const;

    [[nodiscard]] std::vector<nc::NdArray<double>> eval_sweep_abs(Vec3Array const& positions, sweep::Sweep const& sweep) const;

    /**
     * Evaluates the field over a geometry. The number of points for the dimensions is determined form num_params.
     * @param geo geometry to be evaluated
     * @param wavelength wave propagation wavelength
     * @return [array of positions in space, vector of arrays with field values for each position]
     */
    [[nodiscard]] EvalResult<ScalarT> eval_geometry(geometry::Geometry const& geo, double wavelength, std::size_t n_dim1, std::size_t n_dim2) const;


    [[nodiscard]] EvalResult<double> eval_geometry_abs(geometry::Geometry const& geo, double wavelength, std::size_t n_dim1, std::size_t n_dim2) const;

    /**
     * Evaluates the field over a geometry. The number of points for the dimensions is determined form num_params.
     * @param geo geometry to be evaluated
     * @param sweep sweep of wave propagation wavelengths
     * @return [array of positions in space, vector of arrays with field values for each position]
     */
    [[nodiscard]] EvalSweepResult eval_geometry_sweep(geometry::Geometry const& geo, sweep::Sweep const& sweep, std::size_t n_dim1, std::size_t n_dim2) const;

    [[nodiscard]] ArgMaxCurveResult argmax_curve_abs(geometry::Curve const& curve, double wavelength, std::size_t n_points) const;

    [[nodiscard]] ArgMaxSurfaceResult argmax_surface_abs(geometry::Surface const& surf, double wavelength) const;

    [[nodiscard]] CurvePeakSpan find_curve_peak_and_cutoffs(geometry::Curve const& curve, double wavelength, double ratio, std::size_t n_points) const;

    [[nodiscard]] std::pair<Pos, double> calc_beamwidth(geometry::CircleArc const& arc, double wavelength, double ratio, std::size_t n_points) const;

    [[nodiscard]] std::vector<Isoline> trace_isolines(geometry::Geometry const& geo, std::size_t n_dim1, std::size_t n_dim2, double wavelength) const;

    setup::NumParams num_params;

protected:
    // Prevent direct deletion through base pointer without virtual destructor
    ~ScalarField() = default;

    [[nodiscard]] auto make_context() const { return typename Derived::Context(static_cast<Derived const*>(this)); }

    template <typename OutputT, typename OutputCast>
    [[nodiscard]] nc::NdArray<OutputT> eval_impl(Vec3Array const& positions, double wavelength) const;

    template <typename OutputT, typename OutputCast>
    [[nodiscard]] std::vector<nc::NdArray<OutputT>> eval_sweep_impl(Vec3Array const& positions, sweep::Sweep const& sweep) const;
};

template <typename Derived>
using RealScalarField = ScalarField<Derived, double>;

template <typename Derived>
using ComplexScalarField = ScalarField<Derived, Complex>;
