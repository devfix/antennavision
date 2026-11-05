//
// Created by core on 2026-10-31.
//

#pragma once

#include <cmath>
#include <complex>
#include <numbers>
#include <random>
#include <variant>
#include "math/coords.hpp"
#include "math/functions.hpp"

struct PolarPos
{
    double r;
    double polar;
};

struct TTSSpace
{
    [[nodiscard]] static TTSSpace
    create(double wavelength, double tilde_r_p_min, double tilde_r_p_max, double tilde_theta_p_min = -1, double tilde_theta_p_max = +1)
    {
        std::random_device rd;
        return {
            .wavelength = wavelength,
            .generator = std::mt19937(rd()),
            .distr_tilde_r = std::uniform_real_distribution(tilde_r_p_min, tilde_r_p_max),
            .distr_tilde_polar = std::uniform_real_distribution(tilde_theta_p_min, tilde_theta_p_max),
        };
    }

    std::pair<Pos, PolarPos> random_pos()
    {
        double tilde_r = distr_tilde_r(generator);
        double tilde_polar = distr_tilde_polar(generator);
        double r = wavelength / tilde_r;
        double polar = std::acos(tilde_polar);
        Pos pos{r * tilde_polar, r * std::sqrt(1 - math::square(tilde_polar)), 0.0};
        return {pos, {r, polar}};
    }

    double wavelength;
    std::mt19937 generator;
    std::uniform_real_distribution<> distr_tilde_r;
    std::uniform_real_distribution<> distr_tilde_polar;
};

struct CartesianSpace
{
    [[nodiscard]] static CartesianSpace create(double x_min, double x_max, double y_min, double y_max)
    {
        std::random_device rd;
        return {
            .generator = std::mt19937(rd()),
            .distr_x = std::uniform_real_distribution(-x_min, x_max),
            .distr_y = std::uniform_real_distribution(-y_min, y_max),
        };
    }

    std::pair<Pos, PolarPos> random_pos()
    {
        Pos pos{distr_x(generator), distr_y(generator), 0};
        auto [r, _, angle] = math::spherical_from_cartesian_pos(pos);
        return {pos, {r, angle}};
    }

    std::mt19937 generator;
    std::uniform_real_distribution<> distr_x;
    std::uniform_real_distribution<> distr_y;
};

struct PolarSpace
{
    [[nodiscard]] static PolarSpace create(double r_min, double r_max, double polar_min, double polar_max)
    {
        std::random_device rd;
        return {
            .generator = std::mt19937(rd()),
            .distr_r = std::uniform_real_distribution(r_min, r_max),
            .distr_polar = std::uniform_real_distribution(polar_min, polar_max),
        };
    }

    std::pair<Pos, PolarPos> random_pos()
    {
        double r = distr_r(generator);
        double polar = distr_polar(generator);
        Pos pos{r * std::cos(polar), r * std::sin(polar), 0.0};
        return {pos, {r, polar}};
    }

    std::mt19937 generator;
    std::uniform_real_distribution<> distr_r;
    std::uniform_real_distribution<> distr_polar;
};

namespace detail
{
    /// Far-field beam power at normalised Fresnel offset v relative to its peak,
    /// F0(v) = |int_0^1 exp(i*pi/2*v^2*s^2) ds|^2 = (C(v)^2 + S(v)^2) / v^2, evaluated by Simpson's rule.
    [[nodiscard]] inline double far_field_power(double v)
    {
        if (v == 0) return 1.0;
        double const a = std::numbers::pi / 2.0 * v * v;
        int const n = 2 * (200 + static_cast<int>(std::ceil(a)));  // integrand phase changes by a over [0, 1]
        double const h = 1.0 / n;
        std::complex<double> sum = 1.0 + std::polar(1.0, a);
        for (int k = 1; k < n; ++k)
        {
            double const s = k * h;
            sum += (k % 2 ? 4.0 : 2.0) * std::polar(1.0, a * s * s);
        }
        return std::norm(sum * (h / 3.0));
    }

    /// Smallest v > 0 with F0(v) = power_ratio, i.e. the 'v_rho' of the cut-off power ratio rho.
    /// (power_ratio = 1/2 gives 1.318322120.)
    [[nodiscard]] inline double v_rho(double power_ratio)
    {
        if (!(power_ratio > 0 && power_ratio < 1)) throw SimulationError("Invalid power_ratio: {:.15g}", power_ratio);
        double lo = 0, hi = 0.01;
        while (far_field_power(hi) > power_ratio)  // F0 decreases from 1 until its first crossing
        {
            lo = hi;
            hi += 0.01;
            if (hi > 20.0) throw SimulationError("No v_rho found for power_ratio {:.15g}", power_ratio);
        }
        for (int i = 0; i < 80; ++i)
        {
            double const mid = 0.5 * (lo + hi);
            (far_field_power(mid) > power_ratio ? lo : hi) = mid;
        }
        return 0.5 * (lo + hi);
    }
}

struct PolarNearFieldSpace
{
    /// Service area: r_min <= r <= r_crit^max * sin^2(phi), with r_min = rmin_rel * D_A and D_A = (N-1) * d.
    [[nodiscard]] static PolarNearFieldSpace create(std::size_t n_elements, double wavelength, double r_min_rel, double power_ratio, double relative_spacing)
    {
        if (!(r_min_rel > 0)) throw SimulationError("Invalid r_min_rel: {:.15g}", r_min_rel);
        double const v_rho = detail::v_rho(power_ratio);
        double const ta_max = std::sqrt(3)/2.0;
        double const ta_min = -ta_max;
        double const azimuth_min = std::acos(ta_max);
        double const azimuth_max = std::acos(ta_min);
        double const n = static_cast<double>(n_elements);
        double const tr_max = 1.0 / (r_min_rel * relative_spacing * (n - 1.0));  // r~_max = lambda / (rmin_rel * D_A)
        double const r_min = wavelength / tr_max;  // minimal distance rmin_rel * D_A
        double const r_max = wavelength * math::square(relative_spacing) * math::square(n) / (2.0 * math::square(v_rho));  // maximal critical distance r_crit^max
        if (r_min >= r_max) throw SimulationError("r_min {} >= r_max {}", r_min, r_max);

        std::println("PolarNearFieldSpace created: N={}, tr_max={:.15g}, r_min={:.15g}, r_max={:.15g}, azimuth_min={:.15g}, azimuth_max={:.15g}",
            n_elements, tr_max, r_min, r_max, azimuth_min, azimuth_max);

        std::random_device rd;
        return {
            .r_max_boresight = r_max,
            .generator = std::mt19937(rd()),
            .distr_radius = std::uniform_real_distribution(math::square(r_min), math::square(r_max)),
            .distr_azimuth = std::uniform_real_distribution(azimuth_min, azimuth_max),
        };
    }

    [[nodiscard]] std::pair<Pos, PolarPos> random_pos()
    {
        double azimuth{};
        double radius{};
        do
        {
            azimuth = distr_azimuth(generator);
            radius = std::sqrt(distr_radius(generator));
        } while (radius > r_max_boresight * math::square(std::sin(azimuth)));
        Pos pos{radius * std::cos(azimuth), radius * std::sin(azimuth), 0.0};
        return {pos, {radius, azimuth}};
    }

    double r_max_boresight;
    std::mt19937 generator;
    std::uniform_real_distribution<> distr_radius;
    std::uniform_real_distribution<> distr_azimuth;
};

using RandomSpace = std::variant<TTSSpace, CartesianSpace, PolarSpace, PolarNearFieldSpace>;
