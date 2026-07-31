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
        double obj_fn_multi_opt([[maybe_unused]] unsigned n, const double* x, [[maybe_unused]] double* grad, void* data)
        {
            assert(n==N);
            return (*static_cast<decltype(MultiOpt<N>::Params::fn)*>(data))(std::span(x, N));
        }

        double obj_fn_single_opt([[maybe_unused]] unsigned n, const double* x, [[maybe_unused]] double* grad, void* data)
        {
            assert(n==1);
            return (*static_cast<decltype(SingleOpt::Params::fn)*>(data))(*x);
        }

        template <std::size_t N, bool multi_opt>
        nlopt_result run_impl( //
            double const* bounds_lower,
            double const* bounds_upper,
            typename MultiOpt<N>::AnyFn const& fn,
            double* args,
            double* f_min,
            setup::NumParams const& num_params)
        {
            auto opt_deleter = [](nlopt_opt opt)
            {
                nlopt_destroy(opt);
            };
            std::unique_ptr<nlopt_opt_s, decltype(opt_deleter)> opt(nlopt_create(NLOPT_LN_BOBYQA, N), opt_deleter);

            nlopt_set_min_objective(opt.get(), multi_opt ? obj_fn_multi_opt<N> : obj_fn_single_opt, const_cast<MultiOpt<N>::AnyFn*>(&fn));
            nlopt_set_lower_bounds(opt.get(), bounds_lower);
            nlopt_set_upper_bounds(opt.get(), bounds_upper);
            nlopt_set_xtol_rel(opt.get(), num_params.xtol_rel);
            nlopt_set_ftol_rel(opt.get(), num_params.ftol_rel);

            return nlopt_optimize(opt.get(), args, f_min);
        }
    } // namespace

    template <std::size_t N>
    MultiOpt<N>::Result MultiOpt<N>::run(Params const& params, setup::NumParams const& num_params)
    {
        using std::ranges::to;
        using std::ranges::transform;
        using std::views::zip_transform;

        std::array<double, N> bounds_lower{};
        std::ranges::copy(zip_transform(std::ranges::min, params.bounds_a, params.bounds_b), bounds_lower.begin());

        std::array<double, N> bounds_upper{};
        std::ranges::copy(zip_transform(std::ranges::max, params.bounds_a, params.bounds_b), bounds_upper.begin());

        auto opt_deleter = [](nlopt_opt opt)
        {
            nlopt_destroy(opt);
        };
        std::unique_ptr<nlopt_opt_s, decltype(opt_deleter)> opt(nlopt_create(NLOPT_LN_BOBYQA, N), opt_deleter);

        nlopt_set_min_objective(opt.get(), obj_fn_multi_opt<N>, const_cast<decltype(Params::fn)*>(&params.fn));
        nlopt_set_lower_bounds(opt.get(), bounds_lower.data());
        nlopt_set_upper_bounds(opt.get(), bounds_upper.data());
        nlopt_set_xtol_rel(opt.get(), num_params.xtol_rel);
        nlopt_set_ftol_rel(opt.get(), num_params.ftol_rel);

        // we create the result object and initialize the arguments of the minimum with the initial guess, it will be updated by nlopt
        Result result{};
        std::ranges::copy(params.args_initial, result.args_min.begin());

        // if the initial guess is out of bounds, we use the midpoint
        for (std::size_t k = 0; k < N; ++k)
            if (result.args_min[k] < bounds_lower[k] or bounds_upper[k] < result.args_min[k]) result.args_min[k] = std::midpoint(bounds_lower[k], bounds_upper[k]);

        nlopt_result const result_state = nlopt_optimize(opt.get(), result.args_min.data(), &result.f_min);
        if (result_state < 0) throw SimulationError("MultiOpt<{}> failed, nlopt returned '{}'", N, magic_enum::enum_name(result_state));
        return result;
    }

    SingleOpt::Result SingleOpt::run(Params const& params, setup::NumParams const& num_params)
    {
        double const bound_lower = std::min(params.bound_a, params.bound_b);
        double const bound_upper = std::max(params.bound_a, params.bound_b);

        auto opt_deleter = [](nlopt_opt opt)
        {
            nlopt_destroy(opt);
        };
        std::unique_ptr<nlopt_opt_s, decltype(opt_deleter)> opt(nlopt_create(NLOPT_LN_BOBYQA, 1), opt_deleter);

        nlopt_set_min_objective(opt.get(), obj_fn_single_opt, const_cast<decltype(Params::fn)*>(&params.fn));
        nlopt_set_lower_bounds(opt.get(), &bound_lower);
        nlopt_set_upper_bounds(opt.get(), &bound_upper);
        nlopt_set_xtol_rel(opt.get(), num_params.xtol_rel);
        nlopt_set_ftol_rel(opt.get(), num_params.ftol_rel);

        // we create the result object and initialize the argument of the minimum with the initial guess, it will be updated by nlopt
        Result result{.arg_min = params.arg_initial};

        // if the initial guess is out of bounds, we use the midpoint
        if (result.arg_min < bound_lower or bound_upper < result.arg_min) result.arg_min = std::midpoint(bound_lower, bound_upper);

        nlopt_result const result_state = nlopt_optimize(opt.get(), &result.arg_min, &result.f_min);
        if (result_state < 0) throw SimulationError("SingleOpt failed, nlopt returned '{}'", magic_enum::enum_name(result_state));
        return result;
    }

    // -----------------------------------------------------------------------------
    // EXPLICIT INSTANTIATIONS
    // -----------------------------------------------------------------------------
    template struct MultiOpt<1>;
    template struct MultiOpt<2>;
} // namespace eval::opt
