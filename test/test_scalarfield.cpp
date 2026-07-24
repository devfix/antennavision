//
// Created by Tristan Krause on 2026-07-14.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "../include/setup/setup.hpp"
#include "eval/voltagefield.hpp"

TEST_CASE("ScalarField", "[ScalarField]")
{
    ojson const js = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test-ula"
  },
  "num_params": {
    "system_wavelength": 0.1,
    "n_polar": 101,
    "n_azimuth": 201,
    "n_linear1": 101
  },
  "variables": {
    "distance": 100
  },
  "references": [
    {
      "id": "ref_ula",
      "origin": "",
      "rot": { "roll": 0, "pitch": "0.5*pi", "yaw": 0 }
    },
    {
      "id": "ref_rx_start",
      "origin": "",
      "pos": [0, "distance", "-distance/2"]
    },
    {
      "id": "ref_rx_stop",
      "origin": "",
      "pos": [0, "distance", "distance/2"]
    }
  ],
  "antennas": [
    {
      "type": "ULA",
      "id": "ula1",
      "ref": "ref_ula",
      "spacing": "system_wavelength * 0.5",
      "size": 16,
      "rot": { "roll": 0, "pitch": "-0.5*pi", "yaw": 0 },
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
    auto& wavelength = setup->num_params().system_wavelength;
    auto const distance = setup->get_double("distance");
    auto const& tx = setup->get_antenna("ula1");
    auto& rx = setup->get_antenna("receiver");

    auto voltage_field = VoltageField(tx ,rx, setup->num_params());
    {
        auto line = geometry::Line("", pos_t(0, distance, -0.5 * distance), pos_t(0, distance, 0.5 * distance));
        auto result = voltage_field.argmax_curve_abs(line, wavelength);
        REQUIRE((result.pos - pos_t(0.0, distance, 0.0)).norm() == Catch::Approx(0.0).margin(1e-6));
    }
    {
        auto arc = geometry::CircleArc("", POS_ZERO, pos_t(1.0, 0.0, 0.0), pos_t(0.0, distance, 0), POS_ZERO, distance, 0.5 * pi).normalized();
        auto result = voltage_field.argmax_curve_abs(arc, wavelength);
        REQUIRE((result.pos - pos_t(0.0, distance, 0.0)).norm() == Catch::Approx(0.0).margin(1e-6));
    }
    {
        auto arc = geometry::CircleArc("", POS_ZERO, pos_t(1.0, 0.0, 0.0), pos_t(0.0, 1.0, 0.0), POS_ZERO, distance, 0.5 * pi).normalized();
        auto [pos_beam, beamwidth] = voltage_field.calc_beamwidth(arc, wavelength, sqrt2_2);
        REQUIRE(beamwidth == Catch::Approx(0.11053292584412225));
    }
}
