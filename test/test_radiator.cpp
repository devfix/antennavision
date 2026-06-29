//
// Created by Tristan Krause on 2026-05-26.
//

#include <NumCpp/Functions/linspace.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "components/radiator.hpp"
#include "math.hpp"
#include "testutil.hpp"

double constexpr DIPOLE_LENGTH = 1e-3;

TEST_CASE("Mean squared effective length", "[Radiator]")
{
    double constexpr wavelength = 0.1;
    // Hertzian Dipole
    {
        Radiator::elv_spherical_t elv_sherical = [](double const polar, double, double) -> nc::NdArray<complex_t> { return {0, -DIPOLE_LENGTH * std::sin(polar), 0}; };
        double const leffmean = Radiator::calc_mean_squared_effective_length(elv_sherical, wavelength);
        REQUIRE(leffmean == Catch::Approx(2.0 / 3.0 * math::square(DIPOLE_LENGTH)));
    }

    // Half-Wave Dipole
    {
        double constexpr dipole_length = 0.5 * wavelength;
        Radiator::elv_spherical_t elv_sherical = [](double const polar, double, double const wavelength) -> nc::NdArray<complex_t>
        { return Radiator::get_elv_spherical_standing_wave(dipole_length, wavelength, polar); };
        double const leffmean = Radiator::calc_mean_squared_effective_length(elv_sherical, wavelength);
        REQUIRE(leffmean == Catch::Approx(0.5 * math::square(wavelength / pi) * math::q_function(pi)));
    }

    // Full-Wave Dipole
    {
        double constexpr dipole_length = wavelength;
        Radiator::elv_spherical_t elv_sherical = [](double const polar, double, double const wavelength) -> nc::NdArray<complex_t>
        { return Radiator::get_elv_spherical_standing_wave(dipole_length, wavelength, polar); };
        double const leffmean = Radiator::calc_mean_squared_effective_length(elv_sherical, wavelength);
        REQUIRE(leffmean == Catch::Approx(0.5 * math::square(wavelength / pi) * math::q_function(2 * pi)));
    }

    // 3/2-wavelength Dipole
    {
        double constexpr dipole_length = 1.5 * wavelength;
        Radiator::elv_spherical_t elv_sherical = [](double const polar, double, double const wavelength) -> nc::NdArray<complex_t>
        { return Radiator::get_elv_spherical_standing_wave(dipole_length, wavelength, polar); };
        double const leffmean = Radiator::calc_mean_squared_effective_length(elv_sherical, wavelength);
        REQUIRE(leffmean == Catch::Approx(0.5 * math::square(wavelength / pi) * math::q_function(3 * pi)));
    }
}

TEST_CASE("HertzianDipole", "[Radiator]")
{
    Reference const reference("", nullptr);
    auto radiator = Radiator::HertzianDipole::create("HertzianDipole", reference);
    REQUIRE_THROWS(radiator.calc_path(0, 0));

    auto const thetas = nc::linspace(0.0, pi, 21);
    NdArray directivities_actual_phi0(thetas.shape());
    NdArray directivities_actual_phi1(thetas.shape());
    NdArray directivities_expected(thetas.shape());
    std::ranges::transform(thetas, directivities_actual_phi0.begin(), [&radiator](double const theta_) { return radiator.calc_directivity_from_spherical(theta_, 0.0, 101, 201); });
    std::ranges::transform(thetas, directivities_actual_phi1.begin(), [&radiator](double const theta_) { return radiator.calc_directivity_from_spherical(theta_, 1.0, 101, 201); });
    std::ranges::transform(thetas, directivities_expected.begin(), [](double const theta_) { return 1.5 * math::square(std::sin(theta_)); });
    require_close_array(directivities_actual_phi0, directivities_expected);
    require_close_array(directivities_actual_phi1, directivities_expected);
}

TEST_CASE("HalfWaveDipole", "[Radiator]")
{
    double constexpr wavelength = 0.1;
    Reference const reference("", nullptr);
    auto radiator = Radiator::StandingWaveDipole::create("HalfWaveDipole", reference, 0.5 * wavelength);
    REQUIRE_THROWS(radiator.calc_path(0, 0));

    auto const actual = radiator.calc_directivity_from_spherical(0.5 * pi, 0, wavelength);
    REQUIRE(actual == Catch::Approx(1.640922388).margin(1e-3));
}

TEST_CASE("FullWaveDipole", "[Radiator]")
{
    double constexpr wavelength = 0.1;
    Reference const reference("", nullptr);
    auto radiator = Radiator::StandingWaveDipole::create("FullWaveDipole", reference, 1.0 * wavelength);
    REQUIRE_THROWS(radiator.calc_path(0, 0));

    auto const actual = radiator.calc_directivity_from_spherical(0.5 * pi, 0, wavelength);
    REQUIRE(actual == Catch::Approx(2.4116035252).margin(1e-3));
}

TEST_CASE("3/2-WaveDipole", "[Radiator]")
{
    double constexpr wavelength = 0.1;
    Reference const reference("", nullptr);
    auto radiator = Radiator::StandingWaveDipole::create("3/2-WaveDipole", reference, 1.5 * wavelength);
    REQUIRE_THROWS(radiator.calc_path(0, 0));

    auto const actual = radiator.calc_directivity_from_spherical(0.5 * pi, 0, wavelength);
    REQUIRE(actual == Catch::Approx(1.13750300493283).margin(1e-3));
}
