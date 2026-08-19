//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "math/functions.hpp"
#include "math/coords.hpp"
#include "NumCpp/Coordinates/Cartesian.hpp"
#include "NumCpp/Functions/abs.hpp"
#include "NumCpp/Functions/angle.hpp"
#include "eval/rxvoltagefield.hpp"
#include "setup/setup.hpp"
#include "testutil.hpp"

namespace
{
    /**
     * create vector of unity coefficients (uc)
     * @param ant antenna, used to determine correct vector size
     * @return vector of ones
     */
    std::vector<Complex> uc(components::Antenna const& ant) { return std::vector<Complex>(components::antenna::size(ant), 1.0); }
} // namespace

TEST_CASE("ULA position and rotation", "[TestULA]")
{
    ojson const js = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test-ula",
    "version": "0.2.1"
  },
  "sim_params": {
    "system_wavelength": 0.1,
    "n_polar": 101,
    "n_azimuth": 201
  },
  "references": [
    {
      "id": "ref_ula",
      "origin": "",
      "pos": [0, "system_wavelength * 2", "system_wavelength * 2"]
    }
  ],
  "antennas": [
    {
      "type": "ULA",
      "id": "ula1",
      "ref": "ref_ula",
      "rot": { "roll": "0.5*pi", "pitch": 0, "yaw": "0.5*pi" },
      "spacing": "system_wavelength * 0.5",
      "size": 8,
      "radiator": {
        "type": "HertzianDipole"
      }
    }
  ]
}
)JSON");
    auto su = setup::Setup::from_json(js);
    auto const& wavelength = su.sim_params.system_wavelength;
    auto const& ula = components::antenna::cast<components::RadiatorArray>(su.get_antenna("ula1"));

    // check ULA element references
    for (std::size_t i = 0; i < 8; i++)
    {
        auto const& ref_element = ula.get_origin(i);
        double const x = (static_cast<double>(i) - 3.5) * 0.5 * wavelength;
        double const y = 2.0 * wavelength;
        double const z = 2.0 * wavelength;
        CHECK_CLOSE_POSITION(ref_element.global_from_local_pos(POS_ZERO), Pos(x, y, z));
        CHECK_CLOSE_POSITION(ref_element.global_from_local_pos(Pos(wavelength, 0.0, 0.0)), Pos(x, y + wavelength, z));
        CHECK_CLOSE_POSITION(ref_element.global_from_local_pos(Pos(0.0, wavelength, 0.0)), Pos(x, y, z + wavelength));
        CHECK_CLOSE_POSITION(ref_element.global_from_local_pos(Pos(0.0, 0.0, wavelength)), Pos(x + wavelength, y, z));
    }
}

TEST_CASE("ULA gain", "[TestULA]")
{
    ojson const js = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test-ula",
    "version": "0.2.1"
  },
  "sim_params": {
    "system_wavelength": 0.1,
    "n_polar": 101,
    "n_azimuth": 201
  },
  "variables": {
    "distance": 100
  },
  "references": [
    {
      "id": "ref_ula",
      "origin": "",
      "rot": { "yaw": 0, "pitch": "-0.5*pi", "roll": 0}
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
      "size": 3,
      "rot": { "yaw": 0, "pitch": "0.5*pi", "roll": 0 },
      "radiator": {
        "type": "HertzianDipole"
      }
    },
    {
      "id": "receiver",
      "ref": "ref_rx_start",
      "type": "HertzianDipole"
    }
  ]
}
)JSON");
    auto su = setup::Setup::from_json(js);
    auto const& tx = su.get_antenna("ula1");
    auto const& rx = su.get_antenna("receiver");
    reference::Reference const& ref_start = su.get_reference("ref_rx_start");
    reference::Reference const ref_start_initial = ref_start; // we make a copy
    reference::Reference const& ref_stop = su.get_reference("ref_rx_stop");

    constexpr std::size_t n_points = 11;
    Pos const pos_delta = ref_stop.pos - ref_start_initial.pos;
    double const length = pos_delta.norm();

    std::vector<Complex> gains(n_points, 0.0);
    std::vector<double> distances(n_points, 0.0);

    double const* distance_ptr = &ref_start.pos.z;
    for (RealArray::index_type k = 0; k < n_points; k++)
    {
        double const f = static_cast<double>(k) / static_cast<double>(n_points - 1);
        const_cast<Pos&>(ref_start.pos) = ref_start_initial.pos + pos_delta * f; // TODO user better approach than const_cast
        gains.at(k) = components::antenna::calc_voltage_gain(tx, rx, su.sim_params.system_wavelength, uc(tx), uc(rx), su.sim_params);
        distances.at(k) = *distance_ptr;
    }
    const_cast<Pos&>(ref_start.pos) = ref_start_initial.pos; // TODO user better approach than const_cast

    Complex const gain_votage_abs_max = std::ranges::max(gains, {}, [](Complex const& gain) -> double { return std::abs(gain); });
    REQUIRE(std::abs(gain_votage_abs_max) == Catch::Approx(0.00035809851155573));
    REQUIRE(std::arg(gain_votage_abs_max) == Catch::Approx(-0.5 * pi).margin(1e-3));

    std::ranges::transform(gains, gains.begin(), [gain_votage_abs_max](auto gain) -> Complex { return gain / std::abs(gain_votage_abs_max); });

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
    "setup_name": "test-ula",
    "version": "0.2.1"
  },
  "sim_params": {
    "system_wavelength": 0.1,
    "n_polar": 101,
    "n_azimuth": 201,
    "n_linear1": 11
  },
  "variables": {
    "distance": 100
  },
  "references": [
    {
      "id": "ref_ula",
      "origin": "",
      "rot": { "yaw": 0, "pitch": "-0.5*pi", "roll": 0 }
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
      "size": 3,
      "rot": { "yaw": 0, "pitch": "0.5*pi", "roll": 0 },
      "radiator": {
        "type": "HertzianDipole"
      }
    },
    {
      "id": "receiver",
      "ref": "ref_rx_start",
      "type": "HertzianDipole"
    }
  ]
}
)JSON");
    auto su = setup::Setup::from_json(js);
    auto const& tx = su.get_antenna("ula1");
    auto const& rx = su.get_antenna("receiver");
    reference::Reference const& ref_stop = su.get_reference("ref_rx_stop");
    auto voltage_field = eval::RxVoltageField(tx, rx, uc(tx), uc(rx), su.sim_params);

    Pos const pos_start = components::antenna::get_origin(rx)->global_pos();
    Pos const pos_end = ref_stop.global_pos();

    geometry::Line line("", pos_start, pos_end);
    auto result = voltage_field.eval_geometry(line, su.sim_params.system_wavelength, su.sim_params.n_linear1, su.sim_params.n_linear2);
    auto &gains = result.values;
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
