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
double constexpr D0_HERTZIAN = 1.5; /// max directivity of single Hertzian dipole
double constexpr MS_ELV_HERTZIAN = 2.0 / 3.0 * math::square(Radiator::HERTZIAN_DIPOLE_LENGTH); /// ms-elv of single Hertzian dipole

namespace
{
    /**
     * @brief Calculates the exact theoretical mean-squared effective length (Average Power)
     * for a ULA of Z-directed Hertzian dipoles located along the X-axis.
     * @param num_elements Number of elements in the array
     * @param spacing_lambda Element spacing in fractions of a wavelength (e.g., 0.5)
     * @return Exact analytical ms_elv including mutual coupling
     */
    double calc_analytical_ula_ms_elv(std::size_t num_elements, double spacing_lambda, double d0, double single_ms_elv)
    {
        double mutual_coupling_sum = 0.0;

        // Sum the mutual coupling for all pairs of elements
        for (std::size_t m = 1; m < num_elements; ++m)
        {
            // Number of element pairs separated by distance m * d
            double const pair_count = static_cast<double>(num_elements - m);

            // Phase distance between the pairs
            double const kx = m * 2.0 * pi * spacing_lambda;

            // Exact mutual radiation resistance ratio (R_12 / R_11) for parallel Hertzian dipoles
            double const r12_ratio = d0 * (std::sin(kx) / kx + std::cos(kx) / (kx * kx) - std::sin(kx) / (kx * kx * kx));

            mutual_coupling_sum += pair_count * r12_ratio;
        }

        // Total ms_elv = self_power * (N + 2 * sum(mutual_pairs))
        double const ms_elv = single_ms_elv * (static_cast<double>(num_elements) + 2.0 * mutual_coupling_sum);
        return ms_elv;
    }
} // namespace

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
    "version": "1.0.1"
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
    "version": "1.0.1"
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
    "version": "1.0.1"
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
    "version": "1.0.1"
  },
  "sim_params": {
    "system_wavelength": 0.005,
    "n_polar": 50,
    "n_azimuth": 100
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
    auto const& sim_params = su.sim_params();
    double const wavelength = sim_params.system_wavelength;
    antenna::Antenna const& ula = su.get_antenna("DUT");

    std::vector<Complex> const coeffs(antenna::size(ula), 1.0);

    // Target Direction (Broadside), Y-axis = Polar: 90 deg (pi/2), Azimuth: 90 deg (pi/2)
    double constexpr target_polar = pi / 2.0;
    double constexpr target_azimuth = pi / 2.0;

    double const directivity_actual = antenna::calc_directivity_from_spherical(ula, target_polar, target_azimuth, wavelength, coeffs, sim_params);

    // Theoretical expectation: D = N for lambda/2 spaced isotropic elements
    double const directivity_expected = static_cast<double>(antenna::size(ula));

    // epsilon of 1% since the sim params are rather relaxed (not that many integration samples)
    CHECK_THAT(directivity_actual, WithinRel(directivity_expected, 0.01));
}

TEST_CASE("Directivity: 8-Element ULA with Z-Directed Dipoles", "[directivity][ula][dipole]")
{
    // 1. JSON Configuration: Unrotated ULA on X-axis, Z-directed Dipoles
    // Note: We use HertzianDipole (D = 1.5).
    // If your framework has a HalfWaveDipole, you can change the type and set D = 1.64.
    ojson const js = ojson::parse(R"JSON(
    {
      "metadata": {
        "setup_name": "test-directivity",
        "version": "1.0.1"
      },
      "sim_params": {
        "system_wavelength": 0.005,
        "n_polar": 50,
        "n_azimuth": 100
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
          "id": "DUT_DIPOLE",
          "ref": "ref_ula",
          "spacing": "system_wavelength * 0.5",
          "size": 8,
          "radiator": {
            "type": "HertzianDipole"
          }
        }
      ]
    }
    )JSON");

    setup::Setup su(js);
    auto const& sim_params = su.sim_params();
    double const wavelength = sim_params.system_wavelength;
    antenna::Antenna const& ula = su.get_antenna("DUT_DIPOLE");

    std::vector<Complex> const weights(antenna::size(ula), 1.0);

    double const d_max_expected = math::square(antenna::size(ula) * Radiator::HERTZIAN_DIPOLE_LENGTH) / calc_analytical_ula_ms_elv(antenna::size(ula), 0.5, D0_HERTZIAN, MS_ELV_HERTZIAN);

     SECTION("Broadside (Y-Axis) - Maximum Directivity")
    {
        // Y-axis is broadside to the X-axis ULA, and 90-deg (max) to the Z-axis dipole
        double const target_polar = pi / 2.0;
        double const target_azimuth = pi / 2.0;

        double const d_actual = antenna::calc_directivity_from_spherical(ula, target_polar, target_azimuth, wavelength, weights, sim_params);

        // epsilon of 1% since the sim params are rather relaxed (not that many integration samples)
        CHECK_THAT(d_actual, WithinRel(d_max_expected, 0.01));
    }

    SECTION("Dipole Null (Z-Axis)")
    {
        // Z-axis (North pole) is broadside to the Array Factor,
        // BUT it is strictly inside the null of the Z-directed dipoles.
        double const target_polar = 0.0;
        double const target_azimuth = 0.0;

        double const d_actual = antenna::calc_directivity_from_spherical(ula, target_polar, target_azimuth, wavelength, weights, sim_params);

        // Directivity must be physically zero, regardless of the array factor
        CHECK_THAT(d_actual, WithinAbs(0.0, 1e-6));
    }

    SECTION("Off-Axis (Pattern Multiplication Check)")
    {
        // Let's test a random angle: theta = 60 deg, phi = 75 deg
        double const target_polar = 60.0 * (pi / 180.0);
        double const target_azimuth = 75.0 * (pi / 180.0);

        // 1. Analytical Element Pattern (Hertzian Dipole: sin^2(theta))
        double const elem_pattern = math::square(std::sin(target_polar));

        // 2. Analytical Array Factor Magnitude for X-axis array
        // psi = k * d * sin(theta) * cos(phi)
        double const kd = pi; // Because spacing is lambda/2
        double const psi = kd * std::sin(target_polar) * std::cos(target_azimuth);

        // AF = sin(N * psi / 2) / sin(psi / 2)
        double const af_mag = std::abs(std::sin(antenna::size(ula) * psi / 2.0) / std::sin(psi / 2.0));

        // Normalized AF Power
        double const norm_af_power = math::square(af_mag / static_cast<double>(antenna::size(ula)));

        // 3. Expected Pattern Multiplication
        double const d_expected_off_axis = d_max_expected * elem_pattern * norm_af_power;

        double const d_actual = antenna::calc_directivity_from_spherical(ula, target_polar, target_azimuth, wavelength, weights, sim_params);

        // epsilon of 1% since the sim params are rather relaxed (not that many integration samples)
        CHECK_THAT(d_actual, WithinRel(d_expected_off_axis, 0.01));
    }
}
