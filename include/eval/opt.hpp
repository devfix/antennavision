//
// Created by Tristan Krause on 2026-07-29.
//

#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <span>

#include "setup/simparams.hpp"

namespace eval::opt
{

    struct SingleOpt
    {
        struct Params
        {
            double bound_a{};
            double bound_b{};
            std::function<double(double)> fn;
            double arg_initial{};
        };

        struct Result
        {
            double arg_min;
            double f_min;
        };

        static Result run(Params const& params, setup::SimParams const& sim_params);
    };

    template <std::size_t N>
    struct MultiOpt
    {
        struct Params
        {
            std::array<double, N> bounds_a{};
            std::array<double, N> bounds_b{};
            std::function<double(std::span<double const>)> fn;
            std::array<double, N> args_initial{};
        };

        using AnyFn = std::conditional_t<N == 1, decltype(SingleOpt::Params::fn), decltype(MultiOpt<N>::Params::fn)>;

        struct Result
        {
            std::array<double, N> args_min;
            double f_min;
        };

        static Result run(Params const& params, setup::SimParams const& sim_params);
    };

    using DualOpt = MultiOpt<2>;
} // namespace eval::opt
