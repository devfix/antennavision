//
// Created by Tristan Krause on 2026-07-23.
//

#pragma once

#include <utility>
#include "math.hpp"
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

    struct EvalResult
    {
        Vec3Array positions;
        nc::NdArray<ScalarT> values;
    };

    struct EvalSweepResult
    {
        Vec3Array positions;
        std::vector<nc::NdArray<ScalarT>> data;
    };

    struct ArgMaxResult
    {
        double t{};
        Pos pos;
        double max{};
    };

    template <typename DerivedContext>
    struct Context
    {
        [[nodiscard]] Context() = default;

        [[nodiscard]] ScalarT operator()(Pos const& pos, double wavelength) const { return (*static_cast<DerivedContext const*>(this))(pos, wavelength); }
    };

    [[nodiscard]] auto make_context() const { return typename Derived::Context(static_cast<Derived const*>(this)); }

    // CRTP Interface: delegates to Derived::field_impl
    // [[nodiscard]] ScalarT field(Pos const& pos, double wavelength) const
    // {
    //     return static_cast<Derived const*>(this)->field_impl(pos, wavelength);
    // }

    /**
     * Evaluates a field for given positions
     * @param positions array of positions in space
     * @param wavelength wave propagation wavelength
     * @return vector of arrays with field values for each position
     */
    [[nodiscard]] nc::NdArray<ScalarT> eval(Vec3Array const& positions, double wavelength) const;

    /**
     * Evaluates a field for given positions
     * @param positions array of positions in space
     * @param sweep sweep of wave propagation wavelengths
     * @return vector of arrays with field values for each position
     */
    [[nodiscard]] std::vector<nc::NdArray<ScalarT>> eval_sweep(Vec3Array const& positions, sweep::Sweep const& sweep) const;

    /**
     * Evaluates the field over a geometry. The number of points for the dimensions is determined form num_params.
     * @param geo geometry to be evaluated
     * @param wavelength wave propagation wavelength
     * @return [array of positions in space, vector of arrays with field values for each position]
     */
    [[nodiscard]] EvalResult eval_geometry(geometry::Geometry const& geo, std::size_t n_dim1, std::size_t n_dim2, double wavelength) const;
    /**
     * Evaluates the field over a geometry. The number of points for the dimensions is determined form num_params.
     * @param geo geometry to be evaluated
     * @param sweep sweep of wave propagation wavelengths
     * @return [array of positions in space, vector of arrays with field values for each position]
     */
    [[nodiscard]] EvalSweepResult eval_geometry_sweep(geometry::Geometry const& geo, std::size_t n_dim1, std::size_t n_dim2, sweep::Sweep const& sweep) const;

    [[nodiscard]] ArgMaxResult argmax_curve_abs(geometry::Curve const& curve, double wavelength) const;

    [[nodiscard]] CurvePeakSpan find_curve_peak_and_cutoffs(geometry::Curve const& curve, double wavelength, double ratio) const;

    [[nodiscard]] std::pair<Pos, double> calc_beamwidth(geometry::CircleArc const& arc, double wavelength, double ratio) const;

    setup::NumParams num_params;

protected:
    // Prevent direct deletion through base pointer without virtual destructor
    ~ScalarField() = default;
};

template <typename Derived>
using RealScalarField = ScalarField<Derived, double>;

template <typename Derived>
using ComplexScalarField = ScalarField<Derived, Complex>;
