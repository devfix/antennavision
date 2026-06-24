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

TEST_CASE("HertzianDipole Directivity", "[BasicRadiators]")
{
    Reference const reference("", nullptr);
    HertzianDipole radiator("HertzianDipole", reference, DIPOLE_LENGTH);
    REQUIRE_THROWS(radiator.calc_path(0, 0));

    auto const thetas = nc::linspace(0.0, pi, 21);
    NdArray directivities_actual_phi0(thetas.shape());
    NdArray directivities_actual_phi1(thetas.shape());
    NdArray directivities_expected(thetas.shape());
    std::ranges::transform(thetas, directivities_actual_phi0.begin(), [&radiator](double const theta_) { return radiator.calc_directivity(theta_, 0.0, 101, 201); });
    std::ranges::transform(thetas, directivities_actual_phi1.begin(), [&radiator](double const theta_) { return radiator.calc_directivity(theta_, 1.0, 101, 201); });
    std::ranges::transform(thetas, directivities_expected.begin(), [](double const theta_) { return 1.5 * math::square(std::sin(theta_)); });
    require_close_array(directivities_actual_phi0, directivities_expected);
    require_close_array(directivities_actual_phi1, directivities_expected);
}

