//
// Created by Tristan Krause on 2026-08-09.
//

#include <NumCpp/Functions/linspace.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <nlohmann/json.hpp>

#include "components/radiator.hpp"
#include "math/functions.hpp"
#include "setup/setup.hpp"
#include "testutil.hpp"

using Catch::Matchers::WithinRel;

double constexpr DIPOLE_LENGTH = 1e-3;
double constexpr WAVELENGTH = 0.1;

TEST_CASE("Mean squared effective length", "[Radiator]")
{
    auto sim_params = setup::SimParams{.system_wavelength = WAVELENGTH, .n_polar = 101, .n_azimuth = 201};
    SECTION("Hertzian Dipole")
    {
        double const leffmean_expected = 2.0 / 3.0 * math::square(components::Radiator::HERTZIAN_DIPOLE_LENGTH);

        double leffmean_analytical = components::Radiator::HertzianDipole::ms_elv(WAVELENGTH);
        CHECK_THAT(leffmean_analytical, WithinRel(leffmean_expected, 1e-3));

        components::Radiator::elv_spherical_t elv_sherical = [](double polar, double, double) -> nc::NdArray<Complex>
        {
            return {0, -components::Radiator::HERTZIAN_DIPOLE_LENGTH * std::sin(polar), 0};
        };
        double leffmean_numerical = components::Radiator::calc_ms_elv(elv_sherical, WAVELENGTH, sim_params);
        CHECK_THAT(leffmean_numerical, WithinRel(leffmean_expected, 1e-3));
    }

    SECTION("Half-Wave Dipole")
    {
        double constexpr dipole_length = 0.5 * WAVELENGTH;
        double const leffmean_expected = 0.5 * math::square(WAVELENGTH / pi) * math::q_function(pi);

        double leffmean_analytical = components::Radiator::StandingWaveDipole::ms_elv(WAVELENGTH, dipole_length);
        CHECK_THAT(leffmean_analytical, WithinRel(leffmean_expected, 1e-3));

        components::Radiator::elv_spherical_t elv_sherical = [](double polar, [[maybe_unused]] double azimuth, double WAVELENGTH) -> nc::NdArray<Complex>
        {
            return components::Radiator::StandingWaveDipole::elv_spherical(polar, azimuth, WAVELENGTH, dipole_length);
        };
        double leffmean_numerical = components::Radiator::calc_ms_elv(elv_sherical, WAVELENGTH, sim_params);
        CHECK_THAT(leffmean_numerical, WithinRel(leffmean_expected, 1e-3));
    }

    SECTION("Full-Wave Dipole")
    {
        double constexpr dipole_length = WAVELENGTH;
        double const leffmean_expected = 0.5 * math::square(WAVELENGTH / pi) * math::q_function(2 * pi);

        double leffmean_analytical = components::Radiator::StandingWaveDipole::ms_elv(WAVELENGTH, dipole_length);
        CHECK_THAT(leffmean_analytical, WithinRel(leffmean_expected, 1e-3));

        components::Radiator::elv_spherical_t elv_sherical = [](double polar, double azimuth, double WAVELENGTH) -> nc::NdArray<Complex>
        {
            return components::Radiator::StandingWaveDipole::elv_spherical(polar, azimuth, WAVELENGTH, dipole_length);
        };
        double leffmean_numerical = components::Radiator::calc_ms_elv(elv_sherical, WAVELENGTH, sim_params);
        CHECK_THAT(leffmean_numerical, WithinRel(leffmean_expected, 1e-3));
    }

    SECTION("3/2-Wavelength Dipole")
    {
        double constexpr dipole_length = 1.5 * WAVELENGTH;
        double const leffmean_expected = 0.5 * math::square(WAVELENGTH / pi) * math::q_function(3 * pi);

        double leffmean_analytical = components::Radiator::StandingWaveDipole::ms_elv(WAVELENGTH, dipole_length);
        CHECK_THAT(leffmean_analytical, WithinRel(leffmean_expected, 1e-3));

        components::Radiator::elv_spherical_t elv_sherical = [](double polar, double azimuth, double WAVELENGTH) -> nc::NdArray<Complex>
        {
            return components::Radiator::StandingWaveDipole::elv_spherical(polar, azimuth, WAVELENGTH, dipole_length);
        };
        double leffmean_numerical = components::Radiator::calc_ms_elv(elv_sherical, WAVELENGTH, sim_params);
        CHECK_THAT(leffmean_numerical, WithinRel(leffmean_expected, 1e-3));
    }
}
