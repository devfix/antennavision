//
// Created by core on 2026-07-14.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "setup.hpp"


TEST_CASE("ScalarField", "[ScalarField]")
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
        "roll": 0.0,
        "pitch": 0.5,
        "yaw": 0.0
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
      "size": 16,
      "rot": {
        "roll": 0.0,
        "pitch": -0.5,
        "yaw": 0.0
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
    auto const distance = setup->variables.at("distance");
    auto const& tx = setup->get_antenna("ula1");
    auto & rx = setup->get_antenna("receiver");
    math::NumParams num_params{.wavelength = setup->variables.at("wavelength")};
    auto voltage_field = antenna::get_voltage_field(tx, rx, num_params);
    {
        auto [pos_abs_max, _] = voltage_field.argmax_line_abs(pos_t(0, distance, -0.5*distance), pos_t(0, distance, 0.5*distance));
        REQUIRE((pos_abs_max - pos_t(0.0, distance, 0.0)).norm() == Catch::Approx(0.0).margin(1e-6));
    }
    {
        math::Circle circle = math::Circle::make(POS_ZERO, pos_t(1.0, 0.0, 0.0), distance, pos_t(0.0, distance, -0.5*distance));
        auto [pos_abs_max, _] = voltage_field.argmax_circle_abs(circle, 0.5*pi);
        REQUIRE((pos_abs_max - pos_t(0.0, distance, 0.0)).norm() == Catch::Approx(0.0).margin(1e-6));
    }
    {
        math::Circle circle = math::Circle::make(POS_ZERO, pos_t(1.0, 0.0, 0.0), distance, pos_t(0.0, 1.0, 0.0));
        auto [pos_beam, beamwidth] = voltage_field.calc_beamwidth(circle, sqrt2_2);
        REQUIRE(beamwidth == Catch::Approx(0.11053292584412225));
    }
}
