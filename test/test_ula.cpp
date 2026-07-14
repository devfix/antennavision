//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "NumCpp/Coordinates/Cartesian.hpp"
#include "NumCpp/Functions/abs.hpp"
#include "NumCpp/Functions/angle.hpp"
#include "components/radiator.hpp"
#include "math.hpp"
#include "setup.hpp"
#include "testutil.hpp"

TEST_CASE("ULA position and rotation", "[TestULA]")
{
    ojson const js = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test-ula"
  },
  "variables": {
    "wavelength": 0.1
  },
  "references": [
    {
      "id": "ref_ula",
      "origin": "",
      "pos": {
        "x": 0,
        "y": "wavelength * 2",
        "z": "wavelength * 2"
      }
    }
  ],
  "antennas": [
    {
      "type": "ULA",
      "id": "ula1",
      "ref": "ref_ula",
      "rot": {
        "roll": 0.5,
        "pitch": 0.0,
        "yaw": 0.5
      },
      "spacing": "wavelength * 0.5",
      "size": 8,
      "radiator": {
        "type": "HertzianDipole"
      }
    }
  ]
}
)JSON");
    auto const setup = Setup::from_json(js);
    auto const wavelength = setup->variables.at("wavelength");
    auto& ula = antenna::cast<UniformLinearArray>(setup->get_antenna("ula1"));

    // check ULA element references
    for (std::size_t i = 0; i < 8; i++)
    {
        auto const& ref_element = ula.get_reference(i);
        double const x = (static_cast<double>(i) - 3.5) * 0.5 * wavelength;
        double const y = 2.0 * wavelength;
        double const z = 2.0 * wavelength;
        REQUIRE_CLOSE_POSITION(ref_element.global_from_local_pos(POS_ZERO), pos_t(x, y, z));
        REQUIRE_CLOSE_POSITION(ref_element.global_from_local_pos(pos_t(wavelength, 0.0, 0.0)), pos_t(x, y + wavelength, z));
        REQUIRE_CLOSE_POSITION(ref_element.global_from_local_pos(pos_t(0.0, wavelength, 0.0)), pos_t(x, y, z + wavelength));
        REQUIRE_CLOSE_POSITION(ref_element.global_from_local_pos(pos_t(0.0, 0.0, wavelength)), pos_t(x + wavelength, y, z));
    }
}

TEST_CASE("ULA gain", "[TestULA]")
{
    ojson const js = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test-ula"
  },
  "variables": {
    "wavelength": 0.1,
    "distance": 100
  },
  "references": [
    {
      "id": "ref_ula",
      "origin": "",
      "rot": {
        "yaw": 0.0,
        "pitch": -0.5,
        "roll": 0.0
      }
    },
    {
      "id": "ref_rx_start",
      "origin": "",
      "pos": {
        "x": 0,
        "y": "distance",
        "z": "-distance/2"
      }
    },
    {
      "id": "ref_rx_stop",
      "origin": "",
      "pos": {
        "x": 0,
        "y": "distance",
        "z": "distance/2"
      }
    }
  ],
  "antennas": [
    {
      "type": "ULA",
      "id": "ula1",
      "ref": "ref_ula",
      "spacing": "wavelength * 0.5",
      "size": 3,
      "rot": {
        "yaw": 0.0,
        "pitch": 0.5,
        "roll": 0.0
      },
      "radiator": {
        "type": "HertzianDipole"
      }
    },
    {
      "id": "receiver",
      "ref": "ref_rx_start",
      "type": "HertzianDipole"
    }
  ],
  "tasks": [
    {
      "type": "builtin",
      "key": "t00_compare_beamwidth"
    }
  ]
}
)JSON");
    auto const setup = Setup::from_json(js);
    math::NumParams num_params{.wavelength = setup->variables.at("wavelength")};
    auto const& tx = setup->get_antenna("ula1");
    auto const& rx = setup->get_antenna("receiver");
    Reference& ref_start = setup->get_reference("ref_rx_start");
    Reference const& ref_stop = setup->get_reference("ref_rx_stop");

    constexpr std::size_t n_points = 11;
    pos_t const pos_delta = ref_stop.pos - ref_start.pos_initial;
    RealArray const rotation_delta = ref_stop.rotation.toNdArray() - ref_start.rotation.toNdArray();
    double const length = pos_delta.norm();

    std::vector<complex_t> gains(n_points, 0.0);
    std::vector<double> distances(n_points, 0.0);

    double* distance_ptr = &ref_start.pos.z;
    for (RealArray::index_type k = 0; k < n_points; k++)
    {
        double const f = static_cast<double>(k) / static_cast<double>(n_points - 1);
        ref_start.pos = ref_start.pos_initial + pos_delta * f;
        ref_start.rotation = ref_start.rotation_initial.toNdArray() + rotation_delta * f;
        gains.at(k) = antenna::calc_voltage_gain(tx, rx, num_params);
        distances.at(k) = *distance_ptr;
    }
    ref_start.reset();

    complex_t const gain_votage_abs_max = std::ranges::max(gains, {}, [](complex_t const& gain) -> double { return std::abs(gain); });
    REQUIRE(std::abs(gain_votage_abs_max) == Catch::Approx(0.00035809851155573));
    REQUIRE(std::arg(gain_votage_abs_max) == Catch::Approx(-0.5 * pi).margin(1e-3));

    std::ranges::transform(gains, gains.begin(), [gain_votage_abs_max](auto gain) -> complex_t { return gain / std::abs(gain_votage_abs_max); });

    std::vector<double> const gains_power_expected = {0.100653501560284, 0.227131737832402, 0.430093185362579, 0.684406554078239, 0.90888660875903, 1,
                                                      0.90888660875903,  0.684406554078239, 0.430093185362579, 0.227131737832402, 0.100653501560284};
    std::vector<double> const gains_voltage_arg_expected = {-1.78360365074871963, -1.77764329249379149, -1.7634739579509644,  -0.33901331302971988,
                                                            -1.49312253721690102, -1.57131992547758736, -1.49312253721690102, -0.33901331302971988,
                                                            -1.7634739579509644,  -1.77764329249379149, -1.78360365074871963};
    for (std::size_t k = 0; k < gains.size(); k++)
    {
        REQUIRE(math::square(std::abs(gains.at(k))) == Catch::Approx(gains_power_expected.at(k)));
        REQUIRE(std::arg(gains.at(k)) == Catch::Approx(gains_voltage_arg_expected.at(k)));
    }
}

TEST_CASE("ULA gain using ScalarField", "[TestULA]")
{
    ojson const js = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test-ula"
  },
  "variables": {
    "wavelength": 0.1,
    "distance": 100
  },
  "references": [
    {
      "id": "ref_ula",
      "origin": "",
      "rot": {
        "yaw": 0.0,
        "pitch": -0.5,
        "roll": 0.0
      }
    },
    {
      "id": "ref_rx_start",
      "origin": "",
      "pos": {
        "x": 0,
        "y": "distance",
        "z": "-distance/2"
      }
    },
    {
      "id": "ref_rx_stop",
      "origin": "",
      "pos": {
        "x": 0,
        "y": "distance",
        "z": "distance/2"
      }
    }
  ],
  "antennas": [
    {
      "type": "ULA",
      "id": "ula1",
      "ref": "ref_ula",
      "spacing": "wavelength * 0.5",
      "size": 3,
      "rot": {
        "yaw": 0.0,
        "pitch": 0.5,
        "roll": 0.0
      },
      "radiator": {
        "type": "HertzianDipole"
      }
    },
    {
      "id": "receiver",
      "ref": "ref_rx_start",
      "type": "HertzianDipole"
    }
  ],
  "tasks": [
    {
      "type": "builtin",
      "key": "t00_compare_beamwidth"
    }
  ]
}
)JSON");
    auto const setup = Setup::from_json(js);
    math::NumParams num_params{.wavelength = setup->variables.at("wavelength"), .n_linear1 = 11};
    auto const& tx = setup->get_antenna("ula1");
    auto& rx = setup->get_antenna("receiver");
    Reference const& ref_stop = setup->get_reference("ref_rx_stop");
    auto voltage_field = antenna::get_voltage_field(tx, rx, num_params);

    pos_t const pos_start = antenna::get_origin(rx).global_pos();
    pos_t const pos_end = ref_stop.global_pos();

    auto [distances, variant_gains] = voltage_field.eval_line(pos_start, pos_end);
    auto gains = std::get<ComplexArray>(variant_gains);
    auto const gains_abs = nc::abs(gains);
    auto const idx_max = nc::argmax(gains_abs);
    auto const gain_votage_abs_max = gains(idx_max, 0).item();
    REQUIRE(nc::abs(gain_votage_abs_max) == Catch::Approx(0.00035809851155573));
    REQUIRE(nc::angle(gain_votage_abs_max) == Catch::Approx(-0.5 * pi).margin(1e-3));
    gains /= nc::abs(gain_votage_abs_max);

    std::vector<double> const gains_power_expected = {0.100653501560284, 0.227131737832402, 0.430093185362579, 0.684406554078239, 0.90888660875903, 1,
                                                      0.90888660875903,  0.684406554078239, 0.430093185362579, 0.227131737832402, 0.100653501560284};
    std::vector<double> const gains_voltage_arg_expected = {-1.78360365074871963, -1.77764329249379149, -1.7634739579509644,  -0.33901331302971988,
                                                            -1.49312253721690102, -1.57131992547758736, -1.49312253721690102, -0.33901331302971988,
                                                            -1.7634739579509644,  -1.77764329249379149, -1.78360365074871963};
    for (std::size_t k = 0; k < gains.size(); k++)
    {
        REQUIRE(math::square(std::abs(gains.at(k))) == Catch::Approx(gains_power_expected.at(k)));
        REQUIRE(std::arg(gains.at(k)) == Catch::Approx(gains_voltage_arg_expected.at(k)));
    }
}
