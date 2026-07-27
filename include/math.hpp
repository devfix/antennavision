//
// Created by Tristan Krause on 2026-06-03.
//

#pragma once

#include <algorithm>
#include <cmath>
#include <iterator>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include "types/math.hpp"
#include "types/json.hpp"
#include "setup/numparams.hpp"

namespace math
{
    struct OptParams
    {
        std::function<double(double x)> fn; // objective function that gets minimized
        double t_a;
        double t_b;
        setup::NumParams const& num_params;
    };

    struct OptResult
    {
        double t_min{};
        double f_min{};
    };

    struct OptScanResult
    {
        OptResult opt{};
        std::vector<double> scan_t;
        std::vector<double> scan_f;
        std::size_t k_min;
        std::size_t k_lower;
        std::size_t k_upper;
    };

    template <typename R, typename T1, typename T2, typename T3>
    nc::NdArray<R> constexpr vec(T1 a, T2 b, T3 c)
    {
        nc::NdArray<R> vec(3, 1);
        vec(0, 0) = static_cast<R>(a);
        vec(1, 0) = static_cast<R>(b);
        vec(2, 0) = static_cast<R>(c);
        return vec;
    }

    Vec constexpr rotate(Vec const& vec, Quaternion const& quaternion) { return nc::dot(quaternion.toDCM(), vec); }

    double angle_between_vectors(Pos vec1, Pos vec2);

    [[nodiscard]] Pos get_ort_dir(Pos const& dir);

    Quaternion quaternion_from_directions(Pos dir_initial, Pos dir_target);

    std::pair<double, double> sici(double x);

    double q_function(double x);

    /**
     * polar to complex number
     * @param mag magnitude in Euler's plane
     * @param phi angle in Euler's plane
     * @return complex number
     */
    [[nodiscard]] Complex constexpr complex_from_polar(double const mag, double const phi) { return {mag * std::cos(phi), mag * std::sin(phi)}; }

    [[nodiscard]] std::tuple<double, double, double> constexpr spherical_from_cartesian(Pos const& pos)
    {
        double const r = std::hypot(pos.x, pos.y, pos.z);
        if (r < NUMERICAL_MARGIN) { return {0, 0, 0}; }
        double const rho = std::hypot(pos.x, pos.y);
        double const theta = std::atan2(rho, pos.z);
        double const phi = std::atan2(pos.y, pos.x);
        return {r, theta, phi};
    }

    [[nodiscard]] double constexpr db_from_power_ratio(double const power_ratio) { return 10.0 * std::log10(power_ratio); }

    [[nodiscard]] double constexpr power_ratio_from_db(double const db) { return std::pow(10.0, db / 10.0); }

    template <typename T>
    [[nodiscard]] T constexpr square(T x)
    { return x * x; }

    template <typename T>
    [[nodiscard]] double constexpr norm(nc::NdArray<T> array)
    { return std::real(nc::norm(array).item()); }

    /**
     * Computes the spherical-to-Cartesian transformation matrix Omega(theta, phi).
     * The resulting matrix can be used to transform any arbitrary vector from
     * local spherical components [A_r, A_theta, A_phi]^T to global Cartesian
     * components [A_x, A_y, A_z]^T via standard matrix-vector multiplication.
     *
     * @param theta Polar angle (in radians) [cite: 226]
     * @param phi   Azimuthal angle (in radians) [cite: 227]
     * @return      A 3x3 matrix represented as NdArray
     */
    [[nodiscard]] constexpr RealArray get_rot_mat_from_spherical(double const theta, double const phi)
    {
        double const st = std::sin(theta);
        double const ct = std::cos(theta);
        double const sp = std::sin(phi);
        double const cp = std::cos(phi);

        RealArray omega(3, 3);
        omega(0, 0) = st * cp;
        omega(0, 1) = ct * cp;
        omega(0, 2) = -sp;
        omega(1, 0) = st * sp;
        omega(1, 1) = ct * sp;
        omega(1, 2) = cp;
        omega(2, 0) = ct;
        omega(2, 1) = -st;
        omega(2, 2) = 0.0;

        return omega;
    }

    [[nodiscard]] RealArray constexpr get_rot_mat_from_cartesian(Pos const& pos_local)
    {
        auto const [r, polar, azimuth] = spherical_from_cartesian(pos_local);
        return get_rot_mat_from_spherical(polar, azimuth);
    }

    OptResult f_min(OptParams opt_params);

    OptScanResult scan_f_min(OptParams const& opt_params);


    /**
     * Finds the index of the element closest to the target value.
     * Returns std::nullopt if the container is empty.
     */
    template <typename Container, typename T = typename Container::value_type>
    std::optional<std::size_t> find_closest_index(const Container& container, T target)
    {
        if (container.empty()) { return std::nullopt; }
        auto it = std::min_element(
            std::begin(container), std::end(container), [target](const T& a, const T& b) { return std::abs(a - target) < std::abs(b - target); });
        return std::distance(std::begin(container), it);
    }
} // namespace math
