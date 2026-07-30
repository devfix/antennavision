//
// Created by Tristan Krause on 2026-07-29.
//

#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <span>

#include "setup/numparams.hpp"

namespace eval::opt
{
    template <std::size_t N>
    struct MultiOpt
    {
        struct Params
        {
            std::conditional_t<N == 1, double, std::array<double, N>> bounds_a;
            std::conditional_t<N == 1, double, std::array<double, N>> bounds_b;
            std::function<double(std::conditional_t<(N == 1), double const&, std::span<double const> const&>)> fn;
            std::conditional_t<N == 1, double, std::array<double, N>> ts_initial;
        };

        struct Result
        {
            std::conditional_t<N == 1, double, std::array<double, N>> ts_min;
            double f_min;
        };

        static Result run(Params const& params, setup::NumParams const& num_params);
    };

    using SingleOpt = MultiOpt<1>;
    using DualOpt = MultiOpt<2>;
} // namespace eval::opt
