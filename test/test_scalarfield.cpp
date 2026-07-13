//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "components/radiator.hpp"
#include "math.hpp"
#include "setup.hpp"


TEST_CASE("ScalarField", "[TestULA]")
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
  "radiators": [
    {
      "type": "ULA",
      "id": "ula1",
      "ref": "ref_ula",
      "spacing": "wavelength * 0.5",
      "count": 16,
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
    auto const wavelength = setup->variables.at("wavelength");
    auto const distance = setup->variables.at("distance");
    auto const& tx = setup->get_antenna("ula1");
    auto & rx = setup->get_antenna("receiver");
    math::NumParams num_params;
    auto voltage_field = setup->get_voltage_field(tx, rx, num_params);
    {
        auto [pos_abs_max, _] = voltage_field.argmax_line_abs(pos_t(0, distance, -0.5*distance), pos_t(0, distance, 0.5*distance), wavelength);
        REQUIRE((pos_abs_max - pos_t(0.0, distance, 0.0)).norm() == Catch::Approx(0.0).margin(1e-6));
    }
    {
        math::Circle circle = math::get_circle(POS_ZERO, pos_t(1.0, 0.0, 0.0), distance, pos_t(0.0, distance, -0.5*distance));
        auto [pos_abs_max, _] = voltage_field.argmax_circle_abs(circle, 0.5*pi, wavelength);
        REQUIRE((pos_abs_max - pos_t(0.0, distance, 0.0)).norm() == Catch::Approx(0.0).margin(1e-6));
    }
    {
        math::Circle circle = math::get_circle(POS_ZERO, pos_t(1.0, 0.0, 0.0), distance, pos_t(0.0, 1.0, 0.0));
        auto [pos_beam, beamwidth] = voltage_field.calc_beamwidth(circle, sqrt2_2, wavelength);
        REQUIRE(beamwidth == Catch::Approx(0.11053292584412225));
    }
}
