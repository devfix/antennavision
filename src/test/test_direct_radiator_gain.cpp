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
auto constexpr sim_params = setup::SimParams{.system_wavelength = 0.1, .n_polar = N_POLAR, .n_azimuth = N_AZIMUTH};

namespace
{
    /**
     * create vector of unity coefficients (uc)
     * @param ant antenna, used to determine correct vector size
     * @return vector of ones
     */
    std::vector<Complex> uc(antenna::Antenna const& ant) { return std::vector<Complex>(antenna::size(ant), 1.0); }
} // namespace

TEST_CASE("Power Gain: Isotropic Transmitter and Isotropic Receiver")
{
    SECTION("Power Gain: Isotropical Radiators with X-translation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {1000, 0, 0}};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Isotropical Radiators with Y-translation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Isotropical Radiators with z-Translation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {0, 0, 1000}};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinAbs(power_gain_expected, DELTA_MAG));
        // zero magnitude -> no phase check required

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinAbs(power_gain_expected, DELTA_MAG));
        // zero magnitude -> no phase check required
    }

    SECTION("Power Gain: Isotropical Radiators with X-rotation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = {pi / 6.0, 0.0, 0.0}};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Isotropical Radiators with Y-rotation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = {0.0, pi / 6.0, 0.0}};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Isotropical Radiators with Z-rotation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = {0.0, 0.0, pi / 6.0}};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Isotropical Radiators with 3D offset and rotation (1)", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = math::quaternion_from_directions({0, 0, 1}, {1, 1, 1})};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Isotropical Radiators with 3D offset and rotation (2)", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 500}, .rot = math::quaternion_from_directions({0, 0, 1}, {1, 1, 1})};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(2.57681284089676144, DELTA_PHASE));
    }
}

TEST_CASE("Power Gain: Isotropic Transmitter and Hertzian Receiver")
{
    SECTION("Power Gain: Isotropical Radiators with X-translation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {1000, 0, 0}};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.0 * 1.5 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Isotropical Radiators with Y-translation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.0 * 1.5 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Isotropical Radiators with z-Translation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {0, 0, 1000}};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.0 * 0.0 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinAbs(power_gain_expected, DELTA_MAG));
        // zero magnitude -> no phase check required

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinAbs(power_gain_expected, DELTA_MAG));
        // zero magnitude -> no phase check required
    }

    SECTION("Power Gain: Isotropical Radiators with X-rotation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = {pi / 6.0, 0.0, 0.0}};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.0 * 1.125 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Isotropical Radiators with Y-rotation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = {0.0, pi / 6.0, 0.0}};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.0 * 1.5 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Isotropical Radiators with Z-rotation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = {0.0, 0.0, pi / 6.0}};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.0 * 1.5 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Isotropical Radiators with 3D offset and rotation (1)", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = math::quaternion_from_directions({0, 0, 1}, {1, 1, 1})};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.0 * 1.0 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Isotropical Radiators with 3D offset and rotation (2)", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::IsotropicRadiator::create("auto1", "ref1");

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 500}, .rot = math::quaternion_from_directions({0, 0, 1}, {1, 1, 1})};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.0 * 0.6 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(2.57681284089676144, DELTA_PHASE));
    }
}

TEST_CASE("Power Gain: Hertzian Transmitter and Isotropic Receiver")
{
    SECTION("Power Gain: Hertzian dipoles with X-translation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {1000, 0, 0}};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.5 * 1.0 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Hertzian dipoles with Y-translation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.5 * 1.0 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Hertzian dipoles with z-Translation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {0, 0, 1000}};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 0.0 * 1.0 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinAbs(power_gain_expected, DELTA_MAG));
        // zero magnitude -> no phase check required

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinAbs(power_gain_expected, DELTA_MAG));
        // zero magnitude -> no phase check required
    }

    SECTION("Power Gain: Hertzian dipoles with X-rotation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = {pi / 6.0, 0.0, 0.0}};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.5 * 1.0 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Hertzian dipoles with Y-rotation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = {0.0, pi / 6.0, 0.0}};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.5 * 1.0 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Hertzian dipoles with Z-rotation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = {0.0, 0.0, pi / 6.0}};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.5 * 1.0 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Hertzian dipoles with 3D offset and rotation (1)", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = math::quaternion_from_directions({0, 0, 1}, {1, 1, 1})};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.5 * 1.0 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Hertzian dipoles with 3D offset and rotation (2)", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 500}, .rot = math::quaternion_from_directions({0, 0, 1}, {1, 1, 1})};
        antenna::Antenna antenna2 = Radiator::IsotropicRadiator::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.2 * 1.0 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(2.57681284089676144, DELTA_PHASE));
    }
}

TEST_CASE("Power Gain: Hertzian Transmitter and Hertzian Receiver")
{
    SECTION("Power Gain: Hertzian dipoles with X-translation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {1000, 0, 0}};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Hertzian dipoles with Y-translation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Hertzian dipoles with z-Translation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {0, 0, 1000}};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 0.0 * 0.0 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinAbs(power_gain_expected, DELTA_MAG));
        // zero magnitude -> no phase check required

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinAbs(power_gain_expected, DELTA_MAG));
        // zero magnitude -> no phase check required
    }

    SECTION("Power Gain: Hertzian dipoles with X-rotation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = {pi / 6.0, 0.0, 0.0}};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.5 * 1.125 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Hertzian dipoles with Y-rotation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = {0.0, pi / 6.0, 0.0}};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.5 * 1.5 * 0.75 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Hertzian dipoles with Z-rotation", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = {0.0, 0.0, pi / 6.0}};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Hertzian dipoles with 3D offset and rotation (1)", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 0}, .rot = math::quaternion_from_directions({0, 0, 1}, {1, 1, 1})};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.5 * 1.0 * 0.5 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(-0.5 * pi, DELTA_PHASE));
    }

    SECTION("Power Gain: Hertzian dipoles with 3D offset and rotation (2)", "[Gain]")
    {
        reference::Reference ref1{.id = "ref1"};
        antenna::Antenna antenna1 = Radiator::HertzianDipole::create("auto1", "ref1");
        std::get<Radiator>(antenna1).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        reference::Reference ref2{.id = "ref2", .pos = {0, 1000, 500}, .rot = math::quaternion_from_directions({0, 0, 1}, {1, 1, 1})};
        antenna::Antenna antenna2 = Radiator::HertzianDipole::create("auto2", "ref2");
        antenna::rebind_origin_pointers({antenna1, antenna2}, {ref1, ref2});
        std::get<Radiator>(antenna2).ms_elv = nullptr; // disable pre-calculated mean-squared elv to test numerical computation

        double const r = (ref1.global_from_local_pos(POS_ZERO) - ref2.global_from_local_pos(POS_ZERO)).norm();
        double const power_gain_actual = antenna::calc_power_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        double const power_gain_expected = 1.2 * 0.6 * 1.0 / 6.0 * math::square(sim_params.system_wavelength / (4.0 * pi * r)); // Friis equation
        CHECK_THAT(power_gain_actual, WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(math::db_from_power_ratio((power_gain_actual)), WithinRel(math::db_from_power_ratio(power_gain_expected), EPSILON_MAG));

        Complex const voltage_gain_actual =
            antenna::calc_voltage_gain(antenna1, antenna2, sim_params.system_wavelength, uc(antenna1), uc(antenna2), sim_params);
        CHECK_THAT(math::square(std::abs(voltage_gain_actual)), WithinRel(power_gain_expected, EPSILON_MAG));
        CHECK_THAT(std::arg(voltage_gain_actual), WithinAbs(2.57681284089676144, DELTA_PHASE));
    }
}
