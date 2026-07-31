//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "math/functions.hpp"
#include "math/coords.hpp"
#include "components/radiator.hpp"
#include "setup/setup.hpp"
#include "testutil.hpp"

TEST_CASE("Power Gain of auto With X-Translation", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201});
    reference::Reference ref1{.id="ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    reference::Reference ref2{.id="ref2", .pos= {1000, 0, 0}};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(num_params.system_wavelength / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Y-Translation", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201});
    reference::Reference ref1{.id="ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    reference::Reference ref2{.id="ref2",  .pos={0, 1000, 0}};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(num_params.system_wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Z-Translation", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201});
    reference::Reference ref1{.id="ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    reference::Reference ref2{.id="ref2",  .pos={0, 0, 1000}};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    double const power_gain_expected = 0.0 * 0.0 * 1.0 * math::square(num_params.system_wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(0.0));
}

TEST_CASE("Power Gain of auto With X-Rotation", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201});
    reference::Reference ref1{.id="ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    reference::Reference ref2{.id="ref2",  .pos={0, 1000, 0}, .rot={pi / 6.0, 0.0, 0.0}};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    double const power_gain_expected = 1.5 * 1.125 * 1.0 * math::square(num_params.system_wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Y-Rotation", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201});
    reference::Reference ref1{.id="ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    reference::Reference ref2{.id="ref2",  .pos={0, 1000, 0}, .rot={0.0, pi / 6.0, 0.0}};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    double const power_gain_expected = 1.5 * 1.5 * 0.75 * math::square(num_params.system_wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Z-Rotation", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201});
    reference::Reference ref1{.id="ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    reference::Reference ref2{.id="ref2",  .pos={0, 1000, 0}, .rot={0.0, 0.0, pi / 6.0}};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(num_params.system_wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto Complicated 1", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201});
    reference::Reference ref1{.id="ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    reference::Reference ref2{.id="ref2",  .pos={0, 1000, 0}, .rot=math::quaternion_from_directions({0,0,1}, {1,1,1})};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    double const power_gain_expected = 1.5 * 1.0 * 0.5 * math::square(num_params.system_wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto Complicated 2", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = 101, .n_azimuth = 201});
    reference::Reference ref1{.id="ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    reference::Reference ref2{.id="ref2",  .pos={0, 1000, 500}, .rot=math::quaternion_from_directions({0,0,1}, {1,1,1})};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    double const power_gain_expected = 1.2 * 0.6 * 1.0/6.0 * math::square(num_params.system_wavelength  / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params, num_params.system_wavelength);
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(2.57681284089676144));
}