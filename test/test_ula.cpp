//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <nlohmann/json.hpp>
#include "components/radiator.hpp"
#include "math.hpp"
#include "print.hpp"
#include "setup.hpp"
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

    std::vector<complex_t> gains(n_points, 0.0);
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

        for (Radiator const* source : sources) { gains[k] += Radiator::calc_voltage_gain(*source, sink, wavelength, {}); }

        distance = f * length;
        distances[k] = *distance_ptr;
    }
    ref_start.pos = pos_start;
    ref_start.rotation = rotation_start;

    complex_t const gain_votage_abs_max = std::ranges::max(gains, {}, [](complex_t const& gain) -> double { return std::abs(gain); });
    REQUIRE(std::abs(gain_votage_abs_max) == Catch::Approx(0.00035809851155573));
    REQUIRE(std::arg(gain_votage_abs_max) == Catch::Approx(-0.5 * pi).margin(1e-3));

    std::ranges::transform(gains, gains.begin(), [gain_votage_abs_max](auto gain) -> complex_t { return gain / std::abs(gain_votage_abs_max); });

    std::vector<double> const gains_power_expected = {0.100653501560284, 0.227131737832402, 0.430093185362579, 0.684406554078239, 0.90888660875903, 1,
                                                      0.90888660875903,  0.684406554078239, 0.430093185362579, 0.227131737832402, 0.100653501560284};
    std::vector<double> const gains_voltage_arg_expected = {-1.78360365074871963, -1.77764329249379149, -1.7634739579509644, -0.33901331302971988, -1.49312253721690102, -1.57131992547758736,
                                                            -1.49312253721690102, -0.33901331302971988, -1.7634739579509644, -1.77764329249379149, -1.78360365074871963};
    for (std::size_t k = 0; k < gains.size(); k++)
    {
        REQUIRE(math::square(std::abs(gains.at(k))) == Catch::Approx(gains_power_expected.at(k)));
        REQUIRE(std::arg(gains.at(k)) == Catch::Approx(gains_voltage_arg_expected.at(k)));
    }
}
