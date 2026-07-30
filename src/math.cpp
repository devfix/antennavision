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
        template <typename T>
        [[nodiscard]] constexpr double dbl(T val) noexcept
        { return static_cast<double>(val); }

        double objective_function(unsigned n, const double* x, double* grad, void* data)
        {
            auto params = static_cast<OptParams*>(data);
            return params->fn(*x);
        }
    } // namespace

    double angle_between_vectors(Pos vec1, Pos vec2)
    {
        double const norm1 = vec1.norm();
        double const norm2 = vec2.norm();
        if (norm1 < NUMERICAL_MARGIN || norm2 < NUMERICAL_MARGIN) { return 0.0; }
        vec1 /= norm1;
        vec2 /= norm2;
        return std::atan2(vec1.cross(vec2).norm(), vec1.dot(vec2));
    }

    Pos get_ort_dir(Pos const& dir)
    {
        auto const dir_initial = dir.normalize();

        // We need to rotate around an arbitrary axis orthogonal to dir and "search" for a viable orthogonal direction
        // We create the cross-product between dir and each unit vector, these vectors our candidates
        std::array<std::tuple<Pos, double>, 3> dir_orts{{
            {dir_initial.cross(Pos(1, 0, 0)), 0},
            {dir_initial.cross(Pos(0, 1, 0)), 0},
            {dir_initial.cross(Pos(0, 0, 1)), 0} //
        }};
        // for each candidate we determine its norm
        for (auto& [v, len] : dir_orts) { len = v.norm(); }

        // we identify the candidate with the largest norm
        auto const dir_ort_best = std::get<0>(*std::max_element(dir_orts.begin(),
            dir_orts.end(),
            [](std::tuple<Pos, double> const& a, std::tuple<Pos, double> const& b) { return std::get<1>(a) < std::get<1>(b); }));

        // normalize and return the best candidate
        return dir_ort_best.normalize();
    }

    Quaternion quaternion_from_directions(Pos dir_initial, Pos dir_target)
    {
        double const angle = angle_between_vectors(dir_initial, dir_target);

        // case 1: dir_initial and dir_target are equal -> return identity quaternion
        if (std::abs(angle) < NUMERICAL_MARGIN) return {};

        // case 2: angle == +/- pi -> the rotation can take place around any orthogonal axis by angle pi
        if (std::abs(pi - std::abs(angle)) < NUMERICAL_MARGIN) return {get_ort_dir(dir_initial), nc::constants::pi};

        // case 3: angle is not special (no edge case) -> use cross product as orthogonal axis and rotate by angle
        return {dir_initial.cross(dir_target).normalize(), angle};
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

    OptScanResult scan_f_min(OptParams const& opt_params)
    {
        auto const& n_samples = opt_params.num_params.n_linear1;
        OptScanResult opt_result;
        opt_result.scan_t.resize(n_samples);
        opt_result.scan_f.resize(n_samples);
        double const delta = opt_params.t_b - opt_params.t_a;
        for (std::size_t k = 0; k < n_samples; k++)
        {
            double const u = dbl(k) / dbl(n_samples - 1); // u in [0,1]
            double const t = opt_params.t_a + u * delta;
            opt_result.scan_t[k] = t;
            opt_result.scan_f[k] = opt_params.fn(t);
        }
        opt_result.k_min = std::distance(opt_result.scan_f.begin(), std::ranges::min_element(opt_result.scan_f));
        opt_result.k_lower = std::max(static_cast<std::size_t>(0), opt_result.k_min - 1);
        opt_result.k_upper = std::min(n_samples - 1, opt_result.k_min + 1);
        double const u_lower = dbl(opt_result.k_lower) / dbl(n_samples - 1);
        double const u_upper = dbl(opt_result.k_upper) / dbl(n_samples - 1);

        OptParams params_nlopt(opt_params);
        params_nlopt.t_a = opt_params.t_a + u_lower * delta;
        params_nlopt.t_b = opt_params.t_a + u_upper * delta;

        // TODO Fix this
        //opt_result.opt = f_min(params_nlopt); // perform the nl precise optimization

        return opt_result;
    }
} // namespace math
