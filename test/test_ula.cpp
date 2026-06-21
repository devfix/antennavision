//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "../include/setup.hpp"
#include "testutil.hpp"

TEST_CASE("ULA setup", "[TestULA]")
{
    json const js = json::parse(R"JSON(
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
      },
      "rot": {
        "roll": 0,
        "pitch": 0.5,
        "yaw": 0
      }
    }
  ],
  "radiators": [
    {
      "type": "ULA",
      "id": "ula1",
      "ref": "ref_ula",
      "dir": {
        "x": 0,
        "y": 0,
        "z": 1
      },
      "rot": {
        "roll": 0,
        "pitch": 0,
        "yaw": 0.5
      },
      "spacing": "wavelength * 0.5",
      "count": 8,
      "radiator": {
        "type": "HertzianDipole",
        "length": 1e-3
      }
    }
  ]
}
)JSON");
    auto const su = Setup::from_json(js);
    auto const wavelength = su->variables.at("wavelength");

    // check ULA origin references
    {
        auto const &ref_ula = su->get_reference_by_id("ref_ula");
        double const x = 0.0;
        double const y = 2.0 * wavelength;
        double const z = 2.0 * wavelength;
        require_close_position(ref_ula.global_from_local(POS_ZERO), Vec3(x, y, z));
        require_close_position(ref_ula.global_from_local(Vec3(wavelength, 0.0, 0.0)), Vec3(x, y, z - wavelength));
        require_close_position(ref_ula.global_from_local(Vec3(0.0, wavelength, 0.0)), Vec3(x, y + wavelength, z));
        require_close_position(ref_ula.global_from_local(Vec3(0.0, 0.0, wavelength)), Vec3(x + wavelength, y, z));
    }

    // check ULA element references
    for (std::size_t i = 0; i < 8; i++)
    {
        auto const &ref_element = su->get_reference_by_id(std::format("ula1:ref:{}", i));
        double const x = (static_cast<double>(i) - 3.5) * 0.5 * wavelength;
        double const y = 2.0 * wavelength;
        double const z = 2.0 * wavelength;
        require_close_position(ref_element.global_from_local(POS_ZERO), Vec3(x, y, z));
        require_close_position(ref_element.global_from_local(Vec3(wavelength, 0.0, 0.0)), Vec3(x, y + wavelength, z));
        require_close_position(ref_element.global_from_local(Vec3(0.0, wavelength, 0.0)), Vec3(x, y, z + wavelength));
        require_close_position(ref_element.global_from_local(Vec3(0.0, 0.0, wavelength)), Vec3(x + wavelength, y, z));
    }
}
