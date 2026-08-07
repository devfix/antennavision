//
// Created by Tristan Krause on 2026-07-30.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "eval/opt.hpp"

TEST_CASE("SingleOpt finds correct minimum", "[eval::opt][SingleOpt]")
{
    using namespace eval::opt;
    using Catch::Matchers::WithinAbs;

    setup::SimParams const sim_params{};

    SECTION("Find minimum of exp((x-1)^2)")
    {
        SingleOpt::Params params{
            .bound_a = -2,
            .bound_b = 2,
            .fn = [](double x) -> double
            {
                x -= 1;
                return std::exp(x * x);
            },
            .arg_initial = 0 //
        };
        {
            auto const result = SingleOpt::run(params, sim_params);
            CHECK_THAT(result.arg_min, WithinAbs(1, 1e-9));
            CHECK_THAT(result.f_min, WithinAbs(1, 1e-9));
        }

        // swap the bounds and run again
        std::swap(params.bound_a, params.bound_b);
        {
            auto const result = SingleOpt::run(params, sim_params);
            CHECK_THAT(result.arg_min, WithinAbs(1, 1e-9));
            CHECK_THAT(result.f_min, WithinAbs(1, 1e-9));
        }
    }
}

TEST_CASE("DualOpt finds correct minimum", "[eval::opt][DualOpt]")
{
    using namespace eval::opt;
    using Catch::Matchers::WithinAbs;

    setup::SimParams const sim_params{};
    SECTION("Find minimum of sqrt( (x-1)^2 + (y+1)^2 )")
    {
        DualOpt::Params params{
            .bounds_a = {-5, -5},
            .bounds_b = {5, 5},
            .fn = [](std::span<double const> x) -> double { return std::hypot(x[0] - 1, x[1] + 1); },
            .args_initial = {2, 2} //
        };
        {
            auto const result = DualOpt::run(params, sim_params);
            CHECK_THAT(result.args_min[0], WithinAbs(1, 1e-6));
            CHECK_THAT(result.args_min[1], WithinAbs(-1, 1e-6));
            CHECK_THAT(result.f_min, WithinAbs(0, 1e-6));
        }

        // swap the bounds and run again
        std::swap(params.bounds_a, params.bounds_b);
        {
            auto const result = DualOpt::run(params, sim_params);
            CHECK_THAT(result.args_min[0], WithinAbs(1, 1e-6));
            CHECK_THAT(result.args_min[1], WithinAbs(-1, 1e-6));
            CHECK_THAT(result.f_min, WithinAbs(0, 1e-6));
        }
    }
}
