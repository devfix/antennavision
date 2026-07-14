//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "components/radiator.hpp"
#include "math.hpp"
#include "setup.hpp"
#include "testutil.hpp"


TEST_CASE("Power Gain of auto With X-Translation", "[Gain]")
{
    double constexpr wavelength = 0.1;
    Reference reference1("", nullptr);
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {1000, 0, 0});
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.wavelength = wavelength});
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(wavelength / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Y-Translation", "[Gain]")
{
    double constexpr wavelength = 0.1;
    Reference reference1("", nullptr);
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {0, 1000, 0});
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.wavelength = wavelength});
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Z-Translation", "[Gain]")
{
    double constexpr wavelength = 0.1;
    Reference reference1("", nullptr);
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {0, 0, 1000});
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.wavelength = wavelength});
    double const power_gain_expected = 0.0 * 0.0 * 1.0 * math::square(wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(0.0));
}

TEST_CASE("Power Gain of auto With X-Rotation", "[Gain]")
{
    double constexpr wavelength = 0.1;
    Reference reference1("", nullptr);
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {0, 1000, 0}, {pi / 6.0, 0.0, 0.0});
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.wavelength = wavelength});
    double const power_gain_expected = 1.5 * 1.125 * 1.0 * math::square(wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Y-Rotation", "[Gain]")
{
    double constexpr wavelength = 0.1;
    Reference reference1("", nullptr);
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {0, 1000, 0}, {0.0, pi / 6.0, 0.0});
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.wavelength = wavelength});
    double const power_gain_expected = 1.5 * 1.5 * 0.75 * math::square(wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Z-Rotation", "[Gain]")
{
    double constexpr wavelength = 0.1;
    Reference reference1("", nullptr);
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {0, 1000, 0}, {0.0, 0.0, pi / 6.0});
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.wavelength = wavelength});
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto Complicated 1", "[Gain]")
{
    double constexpr wavelength = 0.1;
    Reference reference1("", nullptr);
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {0, 1000, 0}, math::quaternion_from_directions({0,0,1}, {1,1,1}));
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.wavelength = wavelength});
    double const power_gain_expected = 1.5 * 1.0 * 0.5 * math::square(wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto Complicated 2", "[Gain]")
{
    double constexpr wavelength = 0.1;
    Reference reference1("", nullptr);
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {0, 1000, 500}, math::quaternion_from_directions({0,0,1}, {1,1,1}));
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.wavelength = wavelength});
    double const power_gain_expected = 1.2 * 0.6 * 1.0/6.0 * math::square(wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(2.57681284089676144));
}

TEST_CASE("ScalarField", "[Gain]")
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
    auto const wavelength = setup->variables.at("wavelength");
    auto const distance = setup->variables.at("distance");
    auto const& tx = setup->get_antenna("ula1");
    auto & rx = setup->get_antenna("receiver");
    math::NumParams num_params{.wavelength = wavelength};
    auto voltage_field = antenna::get_voltage_field(tx, rx, num_params);
    {
        auto [pos_abs_max, _] = voltage_field.argmax_line_abs(pos_t(0, distance, -0.5*distance), pos_t(0, distance, 0.5*distance));
        REQUIRE((pos_abs_max - pos_t(0.0, distance, 0.0)).norm() == Catch::Approx(0.0).margin(1e-6));
    }
    {
        math::Circle circle = math::get_circle(POS_ZERO, pos_t(1.0, 0.0, 0.0), distance, pos_t(0.0, distance, -0.5*distance));
        auto [pos_abs_max, _] = voltage_field.argmax_circle_abs(circle, 0.5*pi);
        REQUIRE((pos_abs_max - pos_t(0.0, distance, 0.0)).norm() == Catch::Approx(0.0).margin(1e-6));
    }
    {
        math::Circle circle = math::get_circle(POS_ZERO, pos_t(1.0, 0.0, 0.0), distance, pos_t(0.0, 1.0, 0.0));
        auto [pos_beam, beamwidth] = voltage_field.calc_beamwidth(circle, sqrt2_2);
        REQUIRE(beamwidth == Catch::Approx(0.11053292584412225));
    }
}
