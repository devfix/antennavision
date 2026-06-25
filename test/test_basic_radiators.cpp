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

namespace
{
    double q_function(double const x)
    {
        auto const [six, cix] = math::sici(x);
        auto const [si2x, ci2x] = math::sici(2.0 * x);
        return egamma + std::log(x) - cix + 0.5 * std::sin(x) * (si2x - 2.0 * six) + 0.5 * std::cos(x) * (egamma + std::log(0.5 * x) + ci2x - 2.0 * cix);
    }
} // namespace

TEST_CASE("Mean squared effective length", "[BasicRadiators]")
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
        { return Radiator::get_elv_spherical_standing_wave(dipole_length, wavelength, polar, 0.0); };
        double const leffmean = Radiator::calc_mean_squared_effective_length(elv_sherical, wavelength);
        REQUIRE(leffmean == Catch::Approx(0.5 * math::square(wavelength / pi) * q_function(pi)));
    }

    // Full-Wave Dipole
    {
        double constexpr dipole_length = wavelength;
        Radiator::elv_spherical_t elv_sherical = [](double const polar, double, double const wavelength) -> nc::NdArray<complex_t>
        { return Radiator::get_elv_spherical_standing_wave(dipole_length, wavelength, polar, 0.0); };
        double const leffmean = Radiator::calc_mean_squared_effective_length(elv_sherical, wavelength);
        REQUIRE(leffmean == Catch::Approx(0.5 * math::square(wavelength / pi) * q_function(2 * pi)));
    }

    // 3/2-wavelength Dipole
    {
        double constexpr dipole_length = 1.5 * wavelength;
        Radiator::elv_spherical_t elv_sherical = [](double const polar, double, double const wavelength) -> nc::NdArray<complex_t>
        { return Radiator::get_elv_spherical_standing_wave(dipole_length, wavelength, polar, 0.0); };
        double const leffmean = Radiator::calc_mean_squared_effective_length(elv_sherical, wavelength);
        REQUIRE(leffmean == Catch::Approx(0.5 * math::square(wavelength / pi) * q_function(3 * pi)));
    }
}

TEST_CASE("HertzianDipole Directivity", "[BasicRadiators]")
{
    Reference const reference("", nullptr);
    auto radiator = Radiator::HertzianDipole::create("HertzianDipole", reference);
    REQUIRE_THROWS(radiator.calc_path(0, 0));

    auto const thetas = nc::linspace(0.0, pi, 21);
    NdArray directivities_actual_phi0(thetas.shape());
    NdArray directivities_actual_phi1(thetas.shape());
    NdArray directivities_expected(thetas.shape());
    std::ranges::transform(thetas, directivities_actual_phi0.begin(), [&radiator](double const theta_) { return radiator.calc_directivity(theta_, 0.0, 101, 201); });
    std::ranges::transform(thetas, directivities_actual_phi1.begin(), [&radiator](double const theta_) { return radiator.calc_directivity(theta_, 1.0, 101, 201); });
    std::ranges::transform(thetas, directivities_expected.begin(), [](double const theta_) { return 1.5 * math::square(std::sin(theta_)); });
    require_close_array(directivities_actual_phi0, directivities_expected);
    require_close_array(directivities_actual_phi1, directivities_expected);
}
