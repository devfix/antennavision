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

using Catch::Matchers::WithinRel;

double constexpr DIPOLE_LENGTH = 1e-3;
double constexpr WAVELENGTH = 0.1;

TEST_CASE("Mean squared effective length", "[Radiator]")
{
    auto sim_params = setup::SimParams{.system_wavelength = WAVELENGTH, .n_polar = 101, .n_azimuth = 201};
    // Hertzian Dipole
    {
        double const leffmean_expected = 2.0 / 3.0 * math::square(Radiator::HERTZIAN_DIPOLE_LENGTH);

        double leffmean_analytical = Radiator::HertzianDipole::ms_elv(WAVELENGTH);
        CHECK_THAT(leffmean_analytical, WithinRel(leffmean_expected, 1e-3));

        Radiator::elv_spherical_t elv_sherical = [](double polar, double, double) -> nc::NdArray<Complex>
        {
            return {0, -Radiator::HERTZIAN_DIPOLE_LENGTH * std::sin(polar), 0};
        };
        double leffmean_numerical = Radiator::calc_mean_squared_effective_length(elv_sherical, WAVELENGTH, sim_params);
        CHECK_THAT(leffmean_numerical, WithinRel(leffmean_expected, 1e-3));
    }

    // Half-Wave Dipole
    {
        double constexpr dipole_length = 0.5 * WAVELENGTH;
        double const leffmean_expected = 0.5 * math::square(WAVELENGTH / pi) * math::q_function(pi);

        double leffmean_analytical = Radiator::StandingWaveDipole::ms_elv(WAVELENGTH, dipole_length);
        CHECK_THAT(leffmean_analytical, WithinRel(leffmean_expected, 1e-3));

        Radiator::elv_spherical_t elv_sherical = [](double polar, [[maybe_unused]] double azimuth, double WAVELENGTH) -> nc::NdArray<Complex>
        {
            return Radiator::StandingWaveDipole::elv_spherical(polar, azimuth, WAVELENGTH, dipole_length);
        };
        double leffmean_numerical = Radiator::calc_mean_squared_effective_length(elv_sherical, WAVELENGTH, sim_params);
        CHECK_THAT(leffmean_numerical, WithinRel(leffmean_expected, 1e-3));
    }

    // Full-Wave Dipole
    {
        double constexpr dipole_length = WAVELENGTH;
        double const leffmean_expected = 0.5 * math::square(WAVELENGTH / pi) * math::q_function(2 * pi);

        double leffmean_analytical = Radiator::StandingWaveDipole::ms_elv(WAVELENGTH, dipole_length);
        CHECK_THAT(leffmean_analytical, WithinRel(leffmean_expected, 1e-3));

        Radiator::elv_spherical_t elv_sherical = [](double polar, double azimuth, double WAVELENGTH) -> nc::NdArray<Complex>
        {
            return Radiator::StandingWaveDipole::elv_spherical(polar, azimuth, WAVELENGTH, dipole_length);
        };
        double leffmean_numerical = Radiator::calc_mean_squared_effective_length(elv_sherical, WAVELENGTH, sim_params);
        CHECK_THAT(leffmean_numerical, WithinRel(leffmean_expected, 1e-3));
    }

    // 3/2-wavelength Dipole
    {
        double constexpr dipole_length = 1.5 * WAVELENGTH;
        double const leffmean_expected = 0.5 * math::square(WAVELENGTH / pi) * math::q_function(3 * pi);

        double leffmean_analytical = Radiator::StandingWaveDipole::ms_elv(WAVELENGTH, dipole_length);
        CHECK_THAT(leffmean_analytical, WithinRel(leffmean_expected, 1e-3));

        Radiator::elv_spherical_t elv_sherical = [](double polar, double azimuth, double WAVELENGTH) -> nc::NdArray<Complex>
        {
            return Radiator::StandingWaveDipole::elv_spherical(polar, azimuth, WAVELENGTH, dipole_length);
        };
        double leffmean_numerical = Radiator::calc_mean_squared_effective_length(elv_sherical, WAVELENGTH, sim_params);
        CHECK_THAT(leffmean_numerical, WithinRel(leffmean_expected, 1e-3));
    }
}

TEST_CASE("HertzianDipole", "[Radiator]")
{
    auto sim_params = setup::SimParams{.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201};
    reference::Reference reference;
    auto radiator = Radiator::HertzianDipole::create("HertzianDipole", "");
    radiator.origin = &reference;

    auto const thetas = nc::linspace(0.0, pi, 21);
    RealArray directivities_actual_phi0(thetas.shape());
    RealArray directivities_actual_phi1(thetas.shape());
    RealArray directivities_expected(thetas.shape());
    std::ranges::transform(thetas,
        directivities_actual_phi0.begin(),
        [&](double theta_) { return radiator.calc_directivity_from_spherical(theta_, 0.0, WAVELENGTH, sim_params); });
    std::ranges::transform(thetas,
        directivities_actual_phi1.begin(),
        [&](double theta_) { return radiator.calc_directivity_from_spherical(theta_, 1.0, WAVELENGTH, sim_params); });
    std::ranges::transform(thetas, directivities_expected.begin(), [](double theta_) { return 1.5 * math::square(std::sin(theta_)); });
    REQUIRE_CLOSE_ARRAY(directivities_actual_phi0, directivities_expected);
    REQUIRE_CLOSE_ARRAY(directivities_actual_phi1, directivities_expected);
}

TEST_CASE("HalfWaveDipole direct", "[Radiator]")
{
    auto sim_params = setup::SimParams{.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201};
    reference::Reference reference;
    auto radiator = Radiator::StandingWaveDipole::create("HalfWaveDipole", "", 0.5 * sim_params.system_wavelength);
    radiator.origin = &reference;

    auto const actual = radiator.calc_directivity_from_spherical(0.5 * pi, 0, sim_params.system_wavelength, sim_params);
    CHECK_THAT(actual, WithinRel(1.640922388, 1e-3));
}

TEST_CASE("HalfWaveDipole via setup", "[Radiator]")
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
    auto& antenna = setup.get_antenna("DUT");
    auto* radiator = std::get_if<Radiator>(&antenna);
    REQUIRE(radiator);
    auto const actual = radiator->calc_directivity_from_spherical(0.5 * pi, 0, setup.sim_params().system_wavelength, setup.sim_params());
    CHECK_THAT(actual, WithinRel(1.640922388, 1e-3));
}

TEST_CASE("FullWaveDipole direct", "[Radiator]")
{
    auto sim_params = setup::SimParams{.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201};
    reference::Reference reference;
    auto radiator = Radiator::StandingWaveDipole::create("FullWaveDipole", "", 1.0 * sim_params.system_wavelength);
    radiator.origin = &reference;

    auto const actual = radiator.calc_directivity_from_spherical(0.5 * pi, 0, WAVELENGTH, sim_params);
    CHECK_THAT(actual, WithinRel(2.4116035252, 1e-3));
}

TEST_CASE("FullWaveDipole via setup", "[Radiator]")
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
    auto& antenna = setup.get_antenna("DUT");
    auto* radiator = std::get_if<Radiator>(&antenna);
    REQUIRE(radiator);
    auto const actual = radiator->calc_directivity_from_spherical(0.5 * pi, 0, WAVELENGTH, setup.sim_params());
    CHECK_THAT(actual, WithinRel(2.4116035252, 1e-3));
}

TEST_CASE("3/2-WaveDipole direct", "[Radiator]")
{
    auto sim_params = setup::SimParams{.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201};
    reference::Reference reference;
    auto radiator = Radiator::StandingWaveDipole::create("3/2-WaveDipole", "", 1.5 * sim_params.system_wavelength);
    radiator.origin = &reference;

    auto const actual = radiator.calc_directivity_from_spherical(0.5 * pi, 0, WAVELENGTH, sim_params);
    CHECK_THAT(actual, WithinRel(1.13750300493283, 1e-3));
}

TEST_CASE("3/2-WaveDipole via setup", "[Radiator]")
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
    auto& antenna = setup.get_antenna("DUT");
    auto* radiator = std::get_if<Radiator>(&antenna);
    REQUIRE(radiator);
    auto const actual = radiator->calc_directivity_from_spherical(0.5 * pi, 0, WAVELENGTH, setup.sim_params());
    CHECK_THAT(actual, WithinRel(1.13750300493283, 1e-3));
}
