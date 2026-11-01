//
// Created by core on 2026-10-31.
//

#pragma once

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

struct PolarNearFieldSpace
{
    [[nodiscard]] static PolarNearFieldSpace create(std::size_t n_elements, double wavelength, double kappa)
    {
        if (kappa <= 1) throw SimulationError("Invalid kappa: {:.15g}", kappa);
        double constexpr v_rho = 1.318'322'120;
        double const ta_max = std::sqrt(3)/2.0;
        double const ta_min = -ta_max;
        double const azimuth_min = std::acos(ta_max);
        double const azimuth_max = std::acos(ta_min);
        double tr_bs = 4.0 * std::sqrt(math::square(kappa) - 1) / (static_cast<double>(n_elements) - 1.0);
        double r_min = wavelength / tr_bs;  // minimal distance for uniform amplitude at boresight
        double r_max = wavelength / 8.0 * math::square(static_cast<double>(n_elements) / v_rho);  // maximal critical distance r_crit^max
        if (r_min >= r_max) throw SimulationError("r_min {} >= r_max {}", r_min, r_max);

        std::println("PolarNearFieldSpace created: N={}, r_min={:.15g}, r_max={:.15g}, azimuth_min={:.15g}, azimuth_max={:.15g}",
            n_elements, r_min, r_max, azimuth_min, azimuth_max);

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
