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
    Reference ref1{.id="ref1"};
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    Reference ref2{.id="ref2", .pos= {1000, 0, 0}};
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(wavelength / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Y-Translation", "[Gain]")
{
    double constexpr wavelength = 0.1;
    Reference ref1{.id="ref1"};
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    Reference ref2{.id="ref2",  .pos={0, 1000, 0}};
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Z-Translation", "[Gain]")
{
    double constexpr wavelength = 0.1;
    Reference ref1{.id="ref1"};
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    Reference ref2{.id="ref2",  .pos={0, 0, 1000}};
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    double const power_gain_expected = 0.0 * 0.0 * 1.0 * math::square(wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(0.0));
}

TEST_CASE("Power Gain of auto With X-Rotation", "[Gain]")
{
    double constexpr wavelength = 0.1;
    Reference ref1{.id="ref1"};
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    Reference ref2{.id="ref2",  .pos={0, 1000, 0}, .rot={pi / 6.0, 0.0, 0.0}};
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    double const power_gain_expected = 1.5 * 1.125 * 1.0 * math::square(wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Y-Rotation", "[Gain]")
{
    double constexpr wavelength = 0.1;
    Reference ref1{.id="ref1"};
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    Reference ref2{.id="ref2",  .pos={0, 1000, 0}, .rot={0.0, pi / 6.0, 0.0}};
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    double const power_gain_expected = 1.5 * 1.5 * 0.75 * math::square(wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Z-Rotation", "[Gain]")
{
    double constexpr wavelength = 0.1;
    Reference ref1{.id="ref1"};
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    Reference ref2{.id="ref2",  .pos={0, 1000, 0}, .rot={0.0, 0.0, pi / 6.0}};
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto Complicated 1", "[Gain]")
{
    double constexpr wavelength = 0.1;
    Reference ref1{.id="ref1"};
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    Reference ref2{.id="ref2",  .pos={0, 1000, 0}, .rot=math::quaternion_from_directions({0,0,1}, {1,1,1})};
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    double const power_gain_expected = 1.5 * 1.0 * 0.5 * math::square(wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto Complicated 2", "[Gain]")
{
    double constexpr wavelength = 0.1;
    Reference ref1{.id="ref1"};
    Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    Reference ref2{.id="ref2",  .pos={0, 1000, 500}, .rot=math::quaternion_from_directions({0,0,1}, {1,1,1})};
    Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    double const power_gain_expected = 1.2 * 0.6 * 1.0/6.0 * math::square(wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, {.system_wavelength = wavelength});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(2.57681284089676144));
}