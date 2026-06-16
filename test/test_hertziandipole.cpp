//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "components/hertziandipole.hpp"
#include "math.hpp"
#include "testutil.hpp"

double constexpr DIPOLE_LENGTH = 1e-3;

TEST_CASE("HertzianDipole Directivity", "[HertzianDipole]")
{
    Reference const reference("", nullptr);
    HertzianDipole radiator("HertzianDipole", reference, DIPOLE_LENGTH);
    REQUIRE_THROWS(radiator.calc_path(0, 0));

    auto const thetas = nc::linspace(0.0, PI, 21);
    NdArray directivities_actual_phi0(thetas.shape());
    NdArray directivities_actual_phi1(thetas.shape());
    NdArray directivities_expected(thetas.shape());
    std::ranges::transform(thetas, directivities_actual_phi0.begin(), [&radiator](double const theta_) { return radiator.calc_directivity(theta_, 0.0, 101, 201); });
    std::ranges::transform(thetas, directivities_actual_phi1.begin(), [&radiator](double const theta_) { return radiator.calc_directivity(theta_, 1.0, 101, 201); });
    std::ranges::transform(thetas, directivities_expected.begin(), [](double const theta_) { return 1.5 * math::square(std::sin(theta_)); });
    require_close_array(directivities_actual_phi0, directivities_expected);
    require_close_array(directivities_actual_phi1, directivities_expected);
}

TEST_CASE("HertzianDipole Power Gain With X-Translation", "[HertzianDipole]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {1000, 0, 0});
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(lambda / (4.0 * PI * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}

TEST_CASE("HertzianDipole Power Gain With Y-Translation", "[HertzianDipole]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {0, 1000, 0});
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(lambda / (4.0 * PI * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}

TEST_CASE("HertzianDipole Power Gain With Z-Translation", "[HertzianDipole]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {0, 0, 1000});
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 0.0 * 0.0 * 1.0 * math::square(lambda / (4.0 * PI * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}

TEST_CASE("HertzianDipole Power Gain With X-Rotation", "[HertzianDipole]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {0, 1000, 0}, {PI / 6.0, 0.0, 0.0});
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 1.5 * 1.125 * 1.0 * math::square(lambda / (4.0 * PI * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}

TEST_CASE("HertzianDipole Power Gain With Y-Rotation", "[HertzianDipole]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {0, 1000, 0}, {0.0, PI / 6.0, 0.0});
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 1.5 * 1.5 * 0.75 * math::square(lambda / (4.0 * PI * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}

TEST_CASE("HertzianDipole Power Gain With Z-Rotation", "[HertzianDipole]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {0, 1000, 0}, {0.0, 0.0, PI / 6.0});
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 1.5 * 1.5 * 1.0 * math::square(lambda / (4.0 * PI * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}

TEST_CASE("HertzianDipole Power Gain Complicated 1", "[HertzianDipole]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {0, 1000, 0}, math::quaternion_from_directions({0,0,1}, {1,1,1}));
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 1.5 * 1.0 * 0.5 * math::square(lambda / (4.0 * PI * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}

TEST_CASE("HertzianDipole Power Gain Complicated 2", "[HertzianDipole]")
{
    double constexpr lambda = 0.1;
    Reference const reference1("", nullptr);
    HertzianDipole radiator1("HertzianDipole1", reference1, DIPOLE_LENGTH);
    Reference const reference2("", nullptr, {0, 1000, 500}, math::quaternion_from_directions({0,0,1}, {1,1,1}));
    HertzianDipole radiator2("HertzianDipole2", reference2, DIPOLE_LENGTH);

    double const r = (reference1.global_from_local(POS_ZERO) - reference2.global_from_local(POS_ZERO)).norm();
    double const power_gain_actual = radiator1.calc_power_gain(radiator2, lambda);
    double const power_gain_expected = 1.2 * 0.6 * 1.0/6.0 * math::square(lambda / (4.0 * PI * r));
    REQUIRE(power_gain_actual == Catch::Approx(power_gain_expected));
    REQUIRE(math::db_from_power_ratio((power_gain_actual)) == Catch::Approx(math::db_from_power_ratio((power_gain_expected))));
}
