//
// Created by Tristan Krause on 2026-06-03.
//

#include "math.hpp"
#include <cmath>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <nlopt.h>

#include "serialization.hpp"
#include "simulationerror.hpp"

extern "C" {
// part of the Cephes library
extern int sici(double x, double* si, double* ci);
}

namespace math
{
    namespace
    {
        double objective_function(unsigned n, const double* x, double* grad, void* data)
        {
            auto params = static_cast<OptParams*>(data);
            return params->fn(*x);
        }
    } // namespace

    template <any_json_t JsonType>
    void to_json(JsonType& j, NumParams const& num_params)
    {
        j = JsonType{{"wavelength", num_params.wavelength}, {"n_polar", num_params.n_polar}, {"n_azimuth", num_params.n_azimuth},
            {"n_linear1", num_params.n_linear1}, {"n_linear2", num_params.n_linear2}, {"xtol_rel", num_params.xtol_rel}, {"ftol_rel", num_params.ftol_rel}};
    }

    template <any_json_t JsonType>
    void from_json(JsonType const& j, NumParams& num_params)
    {
        serialization::assert_structure(j, "math::NumParams",
            {
                {"wavelength", json::value_t::number_float},
            },
            {
                {"n_polar", json::value_t::number_unsigned},
                {"n_azimuth", json::value_t::number_unsigned},
                {"n_linear1", json::value_t::number_unsigned},
                {"n_linear2", json::value_t::number_unsigned},
                {"xtol_rel", json::value_t::number_float},
                {"ftol_rel", json::value_t::number_float},
            });
        j.at("wavelength").get_to(num_params.wavelength);
        if (j.contains("n_polar")) { j.at("n_polar").get_to(num_params.n_polar); }
        if (j.contains("n_azimuth")) { j.at("n_azimuth").get_to(num_params.n_azimuth); }
        if (j.contains("n_linear1")) { j.at("n_linear1").get_to(num_params.n_linear1); }
        if (j.contains("n_linear2")) { j.at("n_linear2").get_to(num_params.n_linear2); }
        if (j.contains("xtol_rel")) { j.at("xtol_rel").get_to(num_params.xtol_rel); }
        if (j.contains("ftol_rel")) { j.at("ftol_rel").get_to(num_params.ftol_rel); }
    }

    double angle_between_vectors(pos_t vec1, pos_t vec2)
    {
        double const norm1 = vec1.norm();
        double const norm2 = vec2.norm();
        if (norm1 < NUMERICAL_MARGIN || norm2 < NUMERICAL_MARGIN) { return 0.0; }
        vec1 /= norm1;
        vec2 /= norm2;
        return std::atan2(vec1.cross(vec2).norm(), vec1.dot(vec2));
    }

    Quaternion quaternion_from_directions(pos_t dir_initial, pos_t dir_target)
    {
        double const angle = angle_between_vectors(dir_initial, dir_target);
        if (std::abs(angle) < NUMERICAL_MARGIN) { return {}; } // identity quaternion

        if (std::abs(pi - std::abs(angle)) < NUMERICAL_MARGIN)
        {
            // We need to rotate around an arbitrary axis orthogonal to dir_initial.
            dir_initial = dir_initial.normalize();

            // We "search" for a viable orthogonal direction
            std::array<std::tuple<pos_t, double>, 3> dir_orts{
                {{dir_initial.cross(pos_t(1, 0, 0)), 0}, {dir_initial.cross(pos_t(0, 1, 0)), 0}, {dir_initial.cross(pos_t(0, 0, 1)), 0}}};
            for (auto& [dir, len] : dir_orts) { len = dir.norm(); }
            auto const dir_ort_best = std::get<0>(*std::max_element(dir_orts.begin(), dir_orts.end(),
                [](std::tuple<pos_t, double> const& a, std::tuple<pos_t, double> const& b) { return std::get<1>(a) < std::get<1>(b); }));
            return {dir_ort_best.normalize(), nc::constants::pi};
        }
        else
        {
            auto dir_ort = dir_initial.cross(dir_target); // orthogonal axis
            return {dir_ort.normalize(), angle};
        }
    }

    std::pair<double, double> sici(double x)
    {
        double si, ci;
        ::sici(x, &si, &ci);
        return {si, ci};
    }

    double q_function(double const x)
    {
        auto const [six, cix] = math::sici(x);
        auto const [si2x, ci2x] = math::sici(2.0 * x);
        return egamma + std::log(x) - cix + 0.5 * std::sin(x) * (si2x - 2.0 * six) + 0.5 * std::cos(x) * (egamma + std::log(0.5 * x) + ci2x - 2.0 * cix);
    }

    std::pair<double, double> f_min(OptParams const& optimization_params)
    {
        auto const& n_samples = optimization_params.num_params.n_linear1;
        std::vector<double> abs_values(n_samples, 0.0);
        double const delta = optimization_params.x_b - optimization_params.x_a;
        for (std::size_t k = 0; k < n_samples; k++)
        {
            double const f = static_cast<double>(k) / static_cast<double>(n_samples - 1);
            abs_values[k] = optimization_params.fn(optimization_params.x_a + f * delta);
        }
        std::size_t const k_max = std::distance(abs_values.begin(), std::ranges::min_element(abs_values));
        std::size_t const k_a = std::max(static_cast<std::size_t>(0), k_max - 1);
        std::size_t const k_b = std::min(n_samples - 1, k_max + 1);
        double const f_a = static_cast<double>(k_a) / static_cast<double>(n_samples - 1);
        double const f_b = static_cast<double>(k_b) / static_cast<double>(n_samples - 1);

        OptParams params_nlopt(optimization_params);
        params_nlopt.x_a = optimization_params.x_a + f_a * delta;
        params_nlopt.x_b = optimization_params.x_a + f_b * delta;

        double const x_lower = std::min(params_nlopt.x_a, params_nlopt.x_b);
        double const x_upper = std::max(params_nlopt.x_a, params_nlopt.x_b);

        nlopt_opt opt = nlopt_create(NLOPT_LN_BOBYQA, 1); // set algorithm and dimension of x
        nlopt_set_min_objective(opt, objective_function, &params_nlopt);
        nlopt_set_lower_bounds(opt, &x_lower);
        nlopt_set_upper_bounds(opt, &x_upper);
        nlopt_set_xtol_rel(opt, optimization_params.num_params.xtol_rel);
        nlopt_set_ftol_rel(opt, optimization_params.num_params.ftol_rel);
        double x = 0.5 * (params_nlopt.x_a + params_nlopt.x_b); // initial guess
        double min_f;
        nlopt_result const result = nlopt_optimize(opt, &x, &min_f);
        nlopt_destroy(opt);
        if (result < 0) { throw SimulationError("Error: nlopt returned '{}'", magic_enum::enum_name(result)); }
        return {x, min_f};
    }
} // namespace math

template void math::to_json(nlohmann::json&, math::NumParams const&);
template void math::to_json(nlohmann::ordered_json&, math::NumParams const&);
template void math::from_json(nlohmann::json const&, math::NumParams&);
template void math::from_json(nlohmann::ordered_json const&, math::NumParams&);
