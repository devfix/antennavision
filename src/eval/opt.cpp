//
// Created by Tristan Krause on 2026-07-29.
//

#include "eval/opt.hpp"
#include <algorithm>
#include <magic_enum/magic_enum.hpp>
#include <nlopt.h>
#include <ranges>

#include "simulationerror.hpp"

namespace eval::opt
{
    namespace
    {
        template <std::size_t N>
        double objective_function([[maybe_unused]] unsigned n, const double* x, [[maybe_unused]] double* grad, void* data)
        {
            if constexpr (N == 1)
                return static_cast<MultiOpt<N>::Params const*>(data)->fn(*x);
            else
                return static_cast<MultiOpt<N>::Params const*>(data)->fn(std::span(x, N));
        }
    } // namespace

    template <std::size_t N>
    MultiOpt<N>::Result MultiOpt<N>::run(Params const& params, setup::NumParams const& num_params)
    {
        using std::ranges::to;
        using std::views::zip_transform;

        auto const get_span = [](auto& bound)
        {
            if constexpr (N == 1)
                return std::span{&bound, 1};
            else
                return bound;
        };

        std::array<double, N> bounds_lower{};
        std::ranges::copy(zip_transform(std::ranges::min, get_span(params.bounds_a), get_span(params.bounds_b)), bounds_lower.begin());

        std::array<double, N> bounds_upper{};
        std::ranges::copy(zip_transform(std::ranges::max, get_span(params.bounds_a), get_span(params.bounds_b)), bounds_upper.begin());

        std::array<double, N> ts_initial{};
        std::ranges::copy(get_span(params.ts_initial), ts_initial.begin());


        auto opt_deleter = [](nlopt_opt o) { nlopt_destroy(o); };
        std::unique_ptr<nlopt_opt_s, decltype(opt_deleter)> opt(nlopt_create(NLOPT_LN_BOBYQA, N), opt_deleter);

        nlopt_set_min_objective(opt.get(), objective_function<N>, const_cast<Params*>(&params));
        nlopt_set_lower_bounds(opt.get(), bounds_lower.data());
        nlopt_set_upper_bounds(opt.get(), bounds_upper.data());
        nlopt_set_xtol_rel(opt.get(), num_params.xtol_rel);
        nlopt_set_ftol_rel(opt.get(), num_params.ftol_rel);

        Result result{};
        nlopt_result const result_state = nlopt_optimize(opt.get(), ts_initial.data(), &result.f_min);
        if (result_state < 0) { throw SimulationError("Error: nlopt returned '{}'", magic_enum::enum_name(result_state)); }
        if constexpr (N ==1)
            result.ts_min = ts_initial[0];
        else
            std::ranges::copy(ts_initial, result.ts_min.begin());
        return result;
    }

    // -----------------------------------------------------------------------------
    // EXPLICIT INSTANTIATIONS
    // -----------------------------------------------------------------------------
    template struct MultiOpt<1>;
    template struct MultiOpt<2>;
} // namespace eval::opt
