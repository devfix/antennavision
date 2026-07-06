//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "components/radiator.hpp"
#include "math.hpp"
#include "setup.hpp"
#include "testutil.hpp"


TEST_CASE("Power Gain of auto With X-Translation", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference reference1("", nullptr);
    auto radiator1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {1000, 0, 0});
    auto radiator2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = Setup::calc_power_gain(radiator1, radiator2, lambda, {});
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = Setup::calc_voltage_gain(radiator1, radiator2, lambda, {});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Y-Translation", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference reference1("", nullptr);
    auto radiator1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {0, 1000, 0});
    auto radiator2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = Setup::calc_power_gain(radiator1, radiator2, lambda, {});
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = Setup::calc_voltage_gain(radiator1, radiator2, lambda, {});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Z-Translation", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference reference1("", nullptr);
    auto radiator1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {0, 0, 1000});
    auto radiator2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = Setup::calc_power_gain(radiator1, radiator2, lambda, {});
    double const power_gain_expected = 0.0 * 0.0 * 1.0 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = Setup::calc_voltage_gain(radiator1, radiator2, lambda, {});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(0.0));
}

TEST_CASE("Power Gain of auto With X-Rotation", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference reference1("", nullptr);
    auto radiator1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {0, 1000, 0}, {pi / 6.0, 0.0, 0.0});
    auto radiator2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = Setup::calc_power_gain(radiator1, radiator2, lambda, {});
    double const power_gain_expected = 1.5 * 1.125 * 1.0 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = Setup::calc_voltage_gain(radiator1, radiator2, lambda, {});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Y-Rotation", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference reference1("", nullptr);
    auto radiator1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {0, 1000, 0}, {0.0, pi / 6.0, 0.0});
    auto radiator2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = Setup::calc_power_gain(radiator1, radiator2, lambda, {});
    double const power_gain_expected = 1.5 * 1.5 * 0.75 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = Setup::calc_voltage_gain(radiator1, radiator2, lambda, {});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto With Z-Rotation", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference reference1("", nullptr);
    auto radiator1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {0, 1000, 0}, {0.0, 0.0, pi / 6.0});
    auto radiator2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = Setup::calc_power_gain(radiator1, radiator2, lambda, {});
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = Setup::calc_voltage_gain(radiator1, radiator2, lambda, {});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto Complicated 1", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference reference1("", nullptr);
    auto radiator1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {0, 1000, 0}, math::quaternion_from_directions({0,0,1}, {1,1,1}));
    auto radiator2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = Setup::calc_power_gain(radiator1, radiator2, lambda, {});
    double const power_gain_expected = 1.5 * 1.0 * 0.5 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = Setup::calc_voltage_gain(radiator1, radiator2, lambda, {});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(-0.5 * pi));
}

TEST_CASE("Power Gain of auto Complicated 2", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference reference1("", nullptr);
    auto radiator1 = Radiator::HertzianDipole::create("auto1", reference1);
    Reference reference2("", nullptr, {0, 1000, 500}, math::quaternion_from_directions({0,0,1}, {1,1,1}));
    auto radiator2 = Radiator::HertzianDipole::create("auto2", reference2);

    double const r = (reference1.global_from_local_pos(POS_ZERO) - reference2.global_from_local_pos(POS_ZERO)).norm();
    double const power_gain_actual = Setup::calc_power_gain(radiator1, radiator2, lambda, {});
    double const power_gain_expected = 1.2 * 0.6 * 1.0/6.0 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));

    complex_t const voltage_gain_actual = Setup::calc_voltage_gain(radiator1, radiator2, lambda, {});
    REQUIRE(math::square(std::abs(voltage_gain_actual)) == Catch::Approx(power_gain_expected));
    REQUIRE(std::arg(voltage_gain_actual) == Catch::Approx(2.57681284089676144));
}
