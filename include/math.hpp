//
// Created by Tristan Krause on 2026-06-03.
//

#pragma once

#include <sys/stat.h>

#include "types.hpp"

namespace math
{
    template <typename R, typename T>
    nc::NdArray<R> constexpr vec(T a, T b, T c)
    {
        nc::NdArray<R> vec(3, 1);
        vec(0, 0) = static_cast<R>(a);
        vec(1, 0) = static_cast<R>(b);
        vec(2, 0) = static_cast<R>(c);
        return vec;
    }

    vec_t constexpr rotate(vec_t const &vec, nc::rotations::Quaternion const &quaternion) { return nc::dot(quaternion.toDCM(), vec); }

    double angle_between_vectors(Vec3 vec1, Vec3 vec2);
    Quaternion quaternion_from_directions(Vec3 dir_initial, Vec3 dir_target);

    std::pair<double, double> sici(double x);

    /**
     * polar to complex number
     * @param mag magnitude in Euler's plane
     * @param phi angle in Euler's plane
     * @return complex number
     */
    [[nodiscard]] complex_t constexpr complex_from_polar(double const mag, double const phi) { return {mag * std::cos(phi), mag * std::sin(phi)}; }

    [[nodiscard]] std::tuple<double, double, double> constexpr spherical_from_cartesian(Vec3 const &pos)
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
    [[nodiscard]] constexpr NdArray omega(double const theta, double const phi)
    {
        double const st = std::sin(theta);
        double const ct = std::cos(theta);
        double const sp = std::sin(phi);
        double const cp = std::cos(phi);

        NdArray omega(3, 3);
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
} // namespace math
