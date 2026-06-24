//
// Created by Tristan Krause on 2026-05-26.
//

#include <NumCpp/Functions/linspace.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "components/hertziandipole.hpp"
#include "math.hpp"
#include "testutil.hpp"

double constexpr DIPOLE_LENGTH = 1e-3;

TEST_CASE("Power Gain of HertzianDipole With X-Translation", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {1000, 0, 0});
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}

TEST_CASE("Power Gain of HertzianDipole With Y-Translation", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {0, 1000, 0});
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}

TEST_CASE("Power Gain of HertzianDipole With Z-Translation", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {0, 0, 1000});
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 0.0 * 0.0 * 1.0 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}

TEST_CASE("Power Gain of HertzianDipole With X-Rotation", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {0, 1000, 0}, {pi / 6.0, 0.0, 0.0});
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 1.5 * 1.125 * 1.0 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}

TEST_CASE("Power Gain of HertzianDipole With Y-Rotation", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {0, 1000, 0}, {0.0, pi / 6.0, 0.0});
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 1.5 * 1.5 * 0.75 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}

TEST_CASE("Power Gain of HertzianDipole With Z-Rotation", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {0, 1000, 0}, {0.0, 0.0, pi / 6.0});
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}

TEST_CASE("Power Gain of HertzianDipole Complicated 1", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {0, 1000, 0}, math::quaternion_from_directions({0,0,1}, {1,1,1}));
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 1.5 * 1.0 * 0.5 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}

TEST_CASE("Power Gain of HertzianDipole Complicated 2", "[Gain]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {0, 1000, 500}, math::quaternion_from_directions({0,0,1}, {1,1,1}));
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 1.2 * 0.6 * 1.0/6.0 * math::square(lambda / (4.0 * pi * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}
