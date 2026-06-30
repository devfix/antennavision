//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "setup.hpp"
#include "components/radiator.hpp"
#include "math.hpp"
#include "testutil.hpp"

TEST_CASE("ULA position and rotation", "[TestULA]")
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
        "type": "HertzianDipole"
      }
    }
  ]
}
)JSON");
    auto const setup = Setup::from_json(js);
    auto const wavelength = setup->variables.at("wavelength");

    // check ULA origin references
    {
        auto const& ref_ula = setup->get_reference_by_id("ref_ula");
        double const x = 0.0;
        double const y = 2.0 * wavelength;
        double const z = 2.0 * wavelength;
        require_close_position(ref_ula.global_from_local_pos(POS_ZERO), Vec3(x, y, z));
        require_close_position(ref_ula.global_from_local_pos(Vec3(wavelength, 0.0, 0.0)), Vec3(x, y, z - wavelength));
        require_close_position(ref_ula.global_from_local_pos(Vec3(0.0, wavelength, 0.0)), Vec3(x, y + wavelength, z));
        require_close_position(ref_ula.global_from_local_pos(Vec3(0.0, 0.0, wavelength)), Vec3(x + wavelength, y, z));
    }

    // check ULA element references
    for (std::size_t i = 0; i < 8; i++)
    {
        auto const& ref_element = setup->get_reference_by_id(std::format("ula1:ref:{}", i));
        double const x = (static_cast<double>(i) - 3.5) * 0.5 * wavelength;
        double const y = 2.0 * wavelength;
        double const z = 2.0 * wavelength;
        require_close_position(ref_element.global_from_local_pos(POS_ZERO), Vec3(x, y, z));
        require_close_position(ref_element.global_from_local_pos(Vec3(wavelength, 0.0, 0.0)), Vec3(x, y + wavelength, z));
        require_close_position(ref_element.global_from_local_pos(Vec3(0.0, wavelength, 0.0)), Vec3(x, y, z + wavelength));
        require_close_position(ref_element.global_from_local_pos(Vec3(0.0, 0.0, wavelength)), Vec3(x + wavelength, y, z));
    }
}

TEST_CASE("ULA gain", "[TestULA]")
{
    json const js = json::parse(R"JSON(
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
      "origin": ""
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
      "dir": {
        "x": 0,
        "y": 0,
        "z": 1
      },
      "spacing": "wavelength * 0.5",
      "count": 3,
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

    Reference& ref_start = setup->get_reference_by_id("ref_rx_start");
    Vec3 const pos_start = ref_start.pos;
    Reference const& ref_stop = setup->get_reference_by_id("ref_rx_stop");

    constexpr std::size_t n_points = 11;
    NdArray const rotation_start = ref_start.rotation.toNdArray();
    Vec3 const pos_delta = ref_stop.pos - ref_start.pos;
    NdArray const rotation_delta = ref_stop.rotation.toNdArray() - ref_start.rotation.toNdArray();
    double const length = pos_delta.norm();

    std::vector<double> gains(n_points, 0.0);
    std::vector<double> distances(n_points, 0.0);

    auto const& sink = setup->get_radiator_by_id("receiver");
    double distance = 0;

    double* distance_ptr = &ref_start.pos.z;
    auto const sources = setup->get_radiator_array("ula1");
    for (NdArray::index_type k = 0; k < n_points; k++)
    {
        double const f = static_cast<double>(k) / static_cast<double>(n_points - 1);
        ref_start.pos = pos_start + pos_delta * f;
        ref_start.rotation = rotation_start + rotation_delta * f;

        gains[k] = 0;
        complex_t gain = 0;
        for (Radiator const* source : sources) { gain += Radiator::calc_voltage_gain(*source, sink, wavelength); }
        gains[k] = math::square(std::abs(gain));

        distance = f * length;
        distances[k] = *distance_ptr;
    }
    ref_start.pos = pos_start;
    ref_start.rotation = rotation_start;
}
