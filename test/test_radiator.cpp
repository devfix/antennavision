//
// Created by Tristan Krause on 2026-05-26.
//

#include <NumCpp/Functions/linspace.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "../include/setup/setup.hpp"
#include "components/radiator.hpp"
#include "math.hpp"
#include "testutil.hpp"

double constexpr DIPOLE_LENGTH = 1e-3;

TEST_CASE("Mean squared effective length", "[Radiator]")
{
    double constexpr wavelength = 0.1;
    auto num_params = setup::NumParams::configure({.system_wavelength = wavelength, .n_polar = 101, .n_azimuth = 201});
    // Hertzian Dipole
    {
        Radiator::elv_spherical_t elv_sherical = [](double const polar, double, double) -> nc::NdArray<Complex> { return {0, -DIPOLE_LENGTH * std::sin(polar), 0}; };
        double const leffmean = Radiator::calc_mean_squared_effective_length(elv_sherical, num_params);
        REQUIRE(leffmean == Catch::Approx(2.0 / 3.0 * math::square(DIPOLE_LENGTH)));
    }

    // Half-Wave Dipole
    {
        double constexpr dipole_length = 0.5 * wavelength;
        Radiator::elv_spherical_t elv_sherical = [](double const polar, double, double const wavelength) -> nc::NdArray<Complex>
        { return Radiator::get_elv_spherical_standing_wave(dipole_length, wavelength, polar); };
        double const leffmean = Radiator::calc_mean_squared_effective_length(elv_sherical, num_params);
        REQUIRE(leffmean == Catch::Approx(0.5 * math::square(wavelength / pi) * math::q_function(pi)));
    }

    // Full-Wave Dipole
    {
        double constexpr dipole_length = wavelength;
        Radiator::elv_spherical_t elv_sherical = [](double const polar, double, double const wavelength) -> nc::NdArray<Complex>
        { return Radiator::get_elv_spherical_standing_wave(dipole_length, wavelength, polar); };
        double const leffmean = Radiator::calc_mean_squared_effective_length(elv_sherical, num_params);
        REQUIRE(leffmean == Catch::Approx(0.5 * math::square(wavelength / pi) * math::q_function(2 * pi)));
    }

    // 3/2-wavelength Dipole
    {
        double constexpr dipole_length = 1.5 * wavelength;
        Radiator::elv_spherical_t elv_sherical = [](double const polar, double, double const wavelength) -> nc::NdArray<Complex>
        { return Radiator::get_elv_spherical_standing_wave(dipole_length, wavelength, polar); };
        double const leffmean = Radiator::calc_mean_squared_effective_length(elv_sherical, num_params);
        REQUIRE(leffmean == Catch::Approx(0.5 * math::square(wavelength / pi) * math::q_function(3 * pi)));
    }
}

TEST_CASE("HertzianDipole", "[Radiator]")
{
    auto num_params = setup::NumParams::configure({.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201});
    reference::Reference reference;
    auto radiator = Radiator::HertzianDipole::create("HertzianDipole", "");
    radiator.origin = &reference;

    auto const thetas = nc::linspace(0.0, pi, 21);
    RealArray directivities_actual_phi0(thetas.shape());
    RealArray directivities_actual_phi1(thetas.shape());
    RealArray directivities_expected(thetas.shape());
    std::ranges::transform(thetas, directivities_actual_phi0.begin(), [&](double const theta_) { return radiator.calc_directivity_from_spherical(theta_, 0.0, num_params); });
    std::ranges::transform(thetas, directivities_actual_phi1.begin(), [&](double const theta_) { return radiator.calc_directivity_from_spherical(theta_, 1.0, num_params); });
    std::ranges::transform(thetas, directivities_expected.begin(), [](double const theta_) { return 1.5 * math::square(std::sin(theta_)); });
    REQUIRE_CLOSE_ARRAY(directivities_actual_phi0, directivities_expected);
    REQUIRE_CLOSE_ARRAY(directivities_actual_phi1, directivities_expected);
}

TEST_CASE("HalfWaveDipole direct", "[Radiator]")
{
    auto num_params = setup::NumParams::configure({.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201});
    reference::Reference reference;
    auto radiator = Radiator::StandingWaveDipole::create("HalfWaveDipole", "", 0.5 * num_params.system_wavelength);
    radiator.origin = &reference;
    
    auto const actual = radiator.calc_directivity_from_spherical(0.5 * pi, 0, num_params);
    REQUIRE(actual == Catch::Approx(1.640922388).margin(1e-3));
}

TEST_CASE("HalfWaveDipole via setup", "[Radiator]")
{
    ojson const js = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test-radiator"
  },
  "num_params": {
    "system_wavelength": 0.1,
    "n_polar": 101,
    "n_azimuth": 201
  },
  "references": [
    {
      "id": "ref_ula",
      "origin": ""
    }
  ],
  "antennas": [
    {
      "type": "StandingWaveDipole",
      "id": "DUT",
      "ref": "",
      "dipole_length": "0.5*system_wavelength"
    }
  ]
}
)JSON");
    Setup setup(js);
    auto & antenna = setup.get_antenna("DUT");
    auto* radiator = std::get_if<Radiator>(&antenna);
    assert(radiator);
    auto const actual = radiator->calc_directivity_from_spherical(0.5 * pi, 0, setup.num_params());
    REQUIRE(actual == Catch::Approx(1.640922388).margin(1e-3));
}

TEST_CASE("FullWaveDipole direct", "[Radiator]")
{
    auto num_params = setup::NumParams::configure({.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201});
    reference::Reference reference;
    auto radiator = Radiator::StandingWaveDipole::create("FullWaveDipole", "", 1.0 * num_params.system_wavelength);
    radiator.origin = &reference;
    

    auto const actual = radiator.calc_directivity_from_spherical(0.5 * pi, 0, num_params);
    REQUIRE(actual == Catch::Approx(2.4116035252).margin(1e-3));
}

TEST_CASE("FullWaveDipole via setup", "[Radiator]")
{
    ojson const js = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test-radiator"
  },
  "num_params": {
    "system_wavelength": 0.1,
    "n_polar": 101,
    "n_azimuth": 201
  },
  "antennas": [
    {
      "type": "StandingWaveDipole",
      "id": "DUT",
      "ref": "",
      "dipole_length": "1.0*system_wavelength"
    }
  ]
}
)JSON");
    Setup setup(js);
    auto & antenna = setup.get_antenna("DUT");
    auto* radiator = std::get_if<Radiator>(&antenna);
    assert(radiator);
    auto const actual = radiator->calc_directivity_from_spherical(0.5 * pi, 0, setup.num_params());
    REQUIRE(actual == Catch::Approx(2.4116035252).margin(1e-3));
}

TEST_CASE("3/2-WaveDipole direct", "[Radiator]")
{
    auto num_params = setup::NumParams::configure({.system_wavelength = 0.1,.n_polar = 101,.n_azimuth = 201});
    reference::Reference reference;
    auto radiator = Radiator::StandingWaveDipole::create("3/2-WaveDipole", "", 1.5 * num_params.system_wavelength);
    radiator.origin = &reference;
    
    auto const actual = radiator.calc_directivity_from_spherical(0.5 * pi, 0, num_params);
    REQUIRE(actual == Catch::Approx(1.13750300493283).margin(1e-3));
}

TEST_CASE("3/2-WaveDipole via setup", "[Radiator]")
{
    ojson const js = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test-radiator"
  },
  "num_params": {
    "system_wavelength": 0.1,
    "n_polar": 101,
    "n_azimuth": 201
  },
  "references": [
    {
      "id": "ref_ula",
      "origin": ""
    }
  ],
  "antennas": [
    {
      "type": "StandingWaveDipole",
      "id": "DUT",
      "ref": "",
      "dipole_length": "1.5*system_wavelength"
    }
  ]
}
)JSON");
    Setup setup(js);
    auto & antenna = setup.get_antenna("DUT");
    auto* radiator = std::get_if<Radiator>(&antenna);
    assert(radiator);
    auto const actual = radiator->calc_directivity_from_spherical(0.5 * pi, 0, setup.num_params());
    REQUIRE(actual == Catch::Approx(1.13750300493283).margin(1e-3));
}
