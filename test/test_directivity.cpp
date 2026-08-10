//
// Created by Tristan Krause on 2026-05-26.
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

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

double constexpr DIPOLE_LENGTH = 1e-3;
double constexpr WAVELENGTH = 0.1;

TEST_CASE("HertzianDipole", "[Radiator][Directivity]")
{
    using std::ranges::transform;
    auto sim_params = setup::SimParams{.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201};
    reference::Reference reference;
    antenna::Antenna ant = Radiator::HertzianDipole::create("HertzianDipole", "");
    antenna::get_origin(ant) = &reference;
    std::vector<Complex> const coeffs(antenna::size(ant), 1.0);

    auto const thetas = nc::linspace(0.0, pi, 21);
    RealArray directivities_actual_phi0(thetas.shape());
    RealArray directivities_actual_phi1(thetas.shape());
    RealArray directivities_expected(thetas.shape());
    transform(thetas,
        directivities_actual_phi0.begin(),
        [&](double theta_) { return antenna::calc_directivity_from_spherical(ant, theta_, 0.0, WAVELENGTH, coeffs, sim_params); });
    transform(thetas,
        directivities_actual_phi1.begin(),
        [&](double theta_) { return antenna::calc_directivity_from_spherical(ant, theta_, 1.0, WAVELENGTH, coeffs, sim_params); });
    transform(thetas, directivities_expected.begin(), [](double theta_) { return 1.5 * math::square(std::sin(theta_)); });
    REQUIRE_CLOSE_ARRAY(directivities_actual_phi0, directivities_expected);
    REQUIRE_CLOSE_ARRAY(directivities_actual_phi1, directivities_expected);
}

TEST_CASE("HalfWaveDipole direct", "[Radiator][Directivity]")
{
    auto sim_params = setup::SimParams{.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201};
    reference::Reference reference;
    antenna::Antenna ant = Radiator::StandingWaveDipole::create("HalfWaveDipole", "", 0.5 * sim_params.system_wavelength);
    antenna::get_origin(ant) = &reference;
    std::vector<Complex> const coeffs(antenna::size(ant), 1.0);

    auto const actual = antenna::calc_directivity_from_spherical(ant, 0.5 * pi, 0, sim_params.system_wavelength, coeffs, sim_params);
    CHECK_THAT(actual, WithinRel(1.640922388, 1e-3));
}

TEST_CASE("HalfWaveDipole via setup", "[Radiator][Directivity]")
{
    ojson const js = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test-radiator",
    "version": "1.0.0"
  },
  "sim_params": {
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
    setup::Setup setup(js);
    auto& ant = setup.get_antenna("DUT");
    std::vector<Complex> const coeffs(antenna::size(ant), 1.0);
    auto const actual = antenna::calc_directivity_from_spherical(ant, 0.5 * pi, 0, setup.sim_params().system_wavelength, coeffs, setup.sim_params());
    CHECK_THAT(actual, WithinRel(1.640922388, 1e-3));
}

TEST_CASE("FullWaveDipole direct", "[Radiator][Directivity]")
{
    auto sim_params = setup::SimParams{.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201};
    reference::Reference reference;
    antenna::Antenna ant = Radiator::StandingWaveDipole::create("FullWaveDipole", "", 1.0 * sim_params.system_wavelength);
    antenna::get_origin(ant) = &reference;
    std::vector<Complex> const coeffs(antenna::size(ant), 1.0);

    auto const actual = antenna::calc_directivity_from_spherical(ant, 0.5 * pi, 0, WAVELENGTH, coeffs, sim_params);
    CHECK_THAT(actual, WithinRel(2.4116035252, 1e-3));
}

TEST_CASE("FullWaveDipole via setup", "[Radiator][Directivity]")
{
    ojson const js = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test-radiator",
    "version": "1.0.0"
  },
  "sim_params": {
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
    setup::Setup setup(js);
    auto& ant = setup.get_antenna("DUT");
    std::vector<Complex> const coeffs(antenna::size(ant), 1.0);
    auto const actual = antenna::calc_directivity_from_spherical(ant, 0.5 * pi, 0, WAVELENGTH, coeffs, setup.sim_params());
    CHECK_THAT(actual, WithinRel(2.4116035252, 1e-3));
}

TEST_CASE("3/2-WaveDipole direct", "[Radiator][Directivity]")
{
    auto sim_params = setup::SimParams{.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201};
    reference::Reference reference;
    antenna::Antenna ant = Radiator::StandingWaveDipole::create("3/2-WaveDipole", "", 1.5 * sim_params.system_wavelength);
    antenna::get_origin(ant) = &reference;
    std::vector<Complex> const coeffs(antenna::size(ant), 1.0);

    auto const actual = antenna::calc_directivity_from_spherical(ant, 0.5 * pi, 0, WAVELENGTH, coeffs, sim_params);
    CHECK_THAT(actual, WithinRel(1.13750300493283, 1e-3));
}

TEST_CASE("3/2-WaveDipole via setup", "[Radiator][Directivity]")
{
    ojson const js = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test-radiator",
    "version": "1.0.0"
  },
  "sim_params": {
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
    setup::Setup setup(js);
    auto& ant = setup.get_antenna("DUT");
    std::vector<Complex> const coeffs(antenna::size(ant), 1.0);
    auto const actual = antenna::calc_directivity_from_spherical(ant, 0.5 * pi, 0, WAVELENGTH, coeffs, setup.sim_params());
    CHECK_THAT(actual, WithinRel(1.13750300493283, 1e-3));
}

TEST_CASE("Directivity: 8-Element ULA with Isotropic Radiators", "[directivity][ula][array]")
{
    ojson const js = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test-ula",
    "version": "1.0.0"
  },
  "sim_params": {
    "system_wavelength": 0.005,
    "n_polar": 1001,
    "n_azimuth": 2001
  },
  "references": [
    {
      "id": "ref_ula",
      "origin": "",
      "pos": [0, 0, 0]
    }
  ],
  "antennas": [
    {
      "type": "ULA",
      "id": "DUT",
      "ref": "ref_ula",
      "rot": { "roll": "0.5*pi", "pitch": 0, "yaw": "0.5*pi" },
      "spacing": "system_wavelength * 0.5",
      "size": 8,
      "radiator": {
        "type": "IsotropicRadiator"
      }
    }
  ]
}
)JSON");

    setup::Setup su(js);
    auto const & sim_params = su.sim_params();
    double const wavelength = sim_params.system_wavelength;
    antenna::Antenna const& ula = su.get_antenna("DUT");

    std::vector<Complex> const coeffs(antenna::size(ula), 1.0);

    // Target Direction (Broadside), Y-axis = Polar: 90 deg (pi/2), Azimuth: 90 deg (pi/2)
    double constexpr target_polar = pi / 2.0;
    double constexpr target_azimuth = pi / 2.0;

    double const directivity_actual = antenna::calc_directivity_from_spherical(
        ula,
        target_polar,
        target_azimuth,
        wavelength,
        coeffs,
        sim_params
    );

    // Theoretical expectation: D = N for lambda/2 spaced isotropic elements
    double const directivity_expected = static_cast<double>(antenna::size(ula));

    // only epsilon of 1% since the sim params are rather relaxed (not that many integration samples)
    CHECK_THAT(directivity_actual, WithinRel(directivity_expected, 0.01));
}
