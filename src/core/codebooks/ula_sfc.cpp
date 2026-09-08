//
// Created by Tristan Krause on 2026-09-26.
//

#include "codebooks/ula_sfc.hpp"
#include <cmath>
#include <nlohmann/json.hpp>

#include "simulationerror.hpp"
#include "types/math.hpp"

namespace antennavision::codebooks
{

    ULA_SFC ULA_SFC::from_json(json const& js)
    {
        ULA_SFC result{.ula_size = js.at("ula_size").get<std::size_t>()};

        auto const& weights = js.at("codebook");
        result.codebook.reserve(weights.size());

        for (auto const& r_entry : weights)
        {
            double const tilde_r = r_entry.at(0).get<double>();
            auto const& arc_json = r_entry.at(1);

            std::vector<std::pair<double, cb_vector>> arc_cb;
            arc_cb.reserve(arc_json.size());

            for (auto const& theta_entry : arc_json)
            {
                double const tilde_theta = theta_entry.at(0).get<double>();
                auto const weights_comp = theta_entry.at(1).get<std::vector<std::array<double, 2>>>();

                cb_vector coeffs(weights_comp.size());
                std::ranges::transform(weights_comp,
                    coeffs.begin(),
                    [](auto const& w) -> std::complex<double> { return Complex(w.at(0), w.at(1)); });  // math::complex_from_polar

                arc_cb.emplace_back(tilde_theta, std::move(coeffs));
            }

            // validate tilde_theta strictly ascending within this radial slice
            auto const theta_violation = std::ranges::adjacent_find(arc_cb, std::ranges::greater_equal{}, &std::pair<double, cb_vector>::first);
            if (theta_violation != arc_cb.end())
            {
                throw SimulationError("ULA_SFC::from_json: tilde_theta not strictly ascending at tilde_r={:.6f}: {:.6f} >= {:.6f}",
                    tilde_r,
                    theta_violation->first,
                    (theta_violation + 1)->first);
            }

            result.codebook.emplace_back(tilde_r, std::move(arc_cb));
        }

        // validate tilde_r strictly ascending across radial slices
        auto const r_violation =
            std::ranges::adjacent_find(result.codebook, std::ranges::greater_equal{}, &std::pair<double, std::vector<std::pair<double, cb_vector>>>::first);
        if (r_violation != result.codebook.end())
        {
            throw SimulationError("ULA_SFC::from_json: tilde_r not strictly ascending: {:.6f} >= {:.6f}", r_violation->first, (r_violation + 1)->first);
        }

        return result;
    }

    ULA_SFC ULA_SFC::from_file(std::filesystem::path const& p)
    {
        std::ifstream file(p);
        if (!file.is_open()) { throw SimulationError("Could not open codebook. Does the file exist?"); }
        auto const js = json::parse(file);
        file.close();
        return from_json(js);
    }

    ULA_SFC::cb_vector const& ULA_SFC::get_vector(double r_rel, double polar)
    {
        auto const tilde_r_p_min = codebook.front().first;
        auto const tilde_r_p_max = codebook.back().first;
        auto const delta_tilde_r = 0.5 * (tilde_r_p_max - tilde_r_p_min) / static_cast<double>(codebook.size() - 1uz);

        // find r in TTS and its index
        double const tilde_r = 1 / r_rel;
        double const frac_r = (tilde_r - tilde_r_p_min) / (2.0 * delta_tilde_r);
        std::size_t const idx_r = static_cast<std::size_t>(std::round(std::clamp(frac_r, 0.0, static_cast<double>(codebook.size()) - 1.0)));
        auto const& arc_cb = codebook.at(idx_r).second;

        // find polar in TTS and its index
        double const tilde_polar = std::cos(polar);
        auto it = std::ranges::lower_bound(arc_cb, tilde_polar, {}, &std::pair<double, cb_vector>::first);
        std::size_t idx_polar{};
        if (it == arc_cb.begin())
            idx_polar = 0;
        else if (it == arc_cb.end())
            idx_polar = arc_cb.size() - 1;
        else
        {
            double after = it->first;
            double before = (it - 1)->first;

            std::size_t idx_after = it - arc_cb.begin();
            std::size_t idx_before = idx_after - 1;

            idx_polar = (after - tilde_polar < tilde_polar - before) ? idx_after : idx_before;
        }
        return arc_cb.at(idx_polar).second;
    }

} // namespace antennavision::codebooks
