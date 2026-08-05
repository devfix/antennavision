//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "components/radiator.hpp"
#include "math/coords.hpp"
#include "math/functions.hpp"
#include "setup/setup.hpp"
#include "testutil.hpp"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

std::size_t constexpr N_POLAR = 25;
std::size_t constexpr N_AZIMUTH = 2 * N_POLAR;

double constexpr EPSILON_ABS = 1e-4;
double constexpr DELTA_PHASE = 1e-3;

namespace
{
    /**
     * create vector of unity coefficients (uc)
     * @param ant antenna, used to determine correct vector size
     * @return vector of ones
     */
    std::vector<Complex> uc(antenna::Antenna const& ant) { return std::vector<Complex>(antenna::size(ant), 1.0); }
} // namespace

TEST_CASE("Power Gain of auto With X-Translation", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = N_POLAR, .n_azimuth = N_AZIMUTH});
    reference::Reference ref1{.id = "ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    std::get<Radiator>(antenna1).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    reference::Reference ref2{.id = "ref2", .pos = {1000, 0, 0}};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});
    std::get<Radiator>(antenna2).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(num_params.system_wavelength / (4.0 * pi * r));
    CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_ABS));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
}

TEST_CASE("Power Gain of auto With Y-Translation", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = N_POLAR, .n_azimuth = N_AZIMUTH});
    reference::Reference ref1{.id = "ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    std::get<Radiator>(antenna1).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});
    std::get<Radiator>(antenna2).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(num_params.system_wavelength / (4.0 * pi * r));
    CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_ABS));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
}

TEST_CASE("Power Gain of auto With Z-Translation", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = N_POLAR, .n_azimuth = N_AZIMUTH});
    reference::Reference ref1{.id = "ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    std::get<Radiator>(antenna1).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    reference::Reference ref2{.id = "ref2", .pos = {0, 0, 1000}};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});
    std::get<Radiator>(antenna2).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    double const power_gain_expected = 0.0 * 0.0 * 1.0 * math::square(num_params.system_wavelength / (4.0 * pi * r));
    CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_ABS));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(0.0, DELTA_PHASE));
}

TEST_CASE("Power Gain of auto With X-Rotation", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = N_POLAR, .n_azimuth = N_AZIMUTH});
    reference::Reference ref1{.id = "ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    std::get<Radiator>(antenna1).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = {pi / 6.0, 0.0, 0.0}};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});
    std::get<Radiator>(antenna2).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    double const power_gain_expected = 1.5 * 1.125 * 1.0 * math::square(num_params.system_wavelength / (4.0 * pi * r));
    CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_ABS));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
}

TEST_CASE("Power Gain of auto With Y-Rotation", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = N_POLAR, .n_azimuth = N_AZIMUTH});
    reference::Reference ref1{.id = "ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    std::get<Radiator>(antenna1).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = {0.0, pi / 6.0, 0.0}};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});
    std::get<Radiator>(antenna2).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    double const power_gain_expected = 1.5 * 1.5 * 0.75 * math::square(num_params.system_wavelength / (4.0 * pi * r));
    CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_ABS));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
}

TEST_CASE("Power Gain of auto With Z-Rotation", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = N_POLAR, .n_azimuth = N_AZIMUTH});
    reference::Reference ref1{.id = "ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    std::get<Radiator>(antenna1).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = {0.0, 0.0, pi / 6.0}};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});
    std::get<Radiator>(antenna2).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(num_params.system_wavelength / (4.0 * pi * r));
    CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_ABS));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
}

TEST_CASE("Power Gain of auto Complicated 1", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = N_POLAR, .n_azimuth = N_AZIMUTH});
    reference::Reference ref1{.id = "ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    std::get<Radiator>(antenna1).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = math::quaternion_from_directions({0, 0, 1}, {1, 1, 1})};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});
    std::get<Radiator>(antenna2).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    double const power_gain_expected = 1.5 * 1.0 * 0.5 * math::square(num_params.system_wavelength / (4.0 * pi * r));
    CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_ABS));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
}

TEST_CASE("Power Gain of auto Complicated 2", "[Gain]")
{
    auto const num_params = setup::NumParams::create({.system_wavelength = 0.1, .n_polar = N_POLAR, .n_azimuth = N_AZIMUTH});
    reference::Reference ref1{.id = "ref1"};
    antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
    std::get<Radiator>(antenna1).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 500}, .rot = math::quaternion_from_directions({0, 0, 1}, {1, 1, 1})};
    antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
    antenna::resolve_origins({antenna1, antenna2}, {ref1, ref2});
    std::get<Radiator>(antenna2).mean_squared_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

    double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    double const power_gain_expected = 1.2 * 0.6 * 1.0 / 6.0 * math::square(num_params.system_wavelength / (4.0 * pi * r));
    CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_ABS));

    Complex const voltage_gain_actual = antenna::calc_voltage_gain(antenna1, antenna2, num_params.system_wavelength, uc(antenna1), uc(antenna2), num_params);
    CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_ABS));
    CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(2.57681284089676144, DELTA_PHASE));
}
