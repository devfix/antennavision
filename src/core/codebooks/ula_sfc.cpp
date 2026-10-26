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

        // Layout: the outer axis is the angular coordinate tilde_theta =
        // tap_fix (ascending, not assumed uniformly spaced), and each column
        // holds an ascending, likewise not-necessarily-uniform, list of
        // radial coordinates tilde_r = trp_fix.
        auto const& weights = js.at("codebook");
        result.codebook.reserve(weights.size());

        for (auto const& theta_entry : weights)
        {
            double const tilde_theta = theta_entry.at(0).get<double>();
            auto const& radial_json = theta_entry.at(1);

            std::vector<std::pair<double, cb_vector>> radial_cb;
            radial_cb.reserve(radial_json.size());

            for (auto const& r_entry : radial_json)
            {
                double const tilde_r = r_entry.at(0).get<double>();
                auto const weights_comp = r_entry.at(1).get<std::vector<std::array<double, 2>>>();

                cb_vector coeffs(weights_comp.size());
                std::ranges::transform(weights_comp,
                    coeffs.begin(),
                    [](auto const& w) -> std::complex<double> { return Complex(w.at(0), w.at(1)); });  // math::complex_from_polar

                radial_cb.emplace_back(tilde_r, std::move(coeffs));
            }

            // validate tilde_r strictly ascending within this angular column
            auto const r_violation = std::ranges::adjacent_find(radial_cb, std::ranges::greater_equal{}, &std::pair<double, cb_vector>::first);
            if (r_violation != radial_cb.end())
            {
                throw SimulationError("ULA_SFC::from_json: tilde_r not strictly ascending at tilde_theta={:.6f}: {:.6f} >= {:.6f}",
                    tilde_theta,
                    r_violation->first,
                    (r_violation + 1)->first);
            }

            result.codebook.emplace_back(tilde_theta, std::move(radial_cb));
        }

        // validate tilde_theta strictly ascending across angular columns
        auto const theta_violation =
            std::ranges::adjacent_find(result.codebook, std::ranges::greater_equal{}, &std::pair<double, std::vector<std::pair<double, cb_vector>>>::first);
        if (theta_violation != result.codebook.end())
        {
            throw SimulationError(
                "ULA_SFC::from_json: tilde_theta not strictly ascending: {:.6f} >= {:.6f}", theta_violation->first, (theta_violation + 1)->first);
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
        // The angular axis (tilde_theta) is not assumed to be uniformly
        // spaced (e.g. extra columns may be added near endfire on top of an
        // otherwise-regular grid), so binary-search for the closest
        // ascending entry instead of rounding via an assumed fixed spacing.
        double const tilde_theta = std::cos(polar);
        auto theta_it = std::ranges::lower_bound(codebook, tilde_theta, {}, &std::pair<double, std::vector<std::pair<double, cb_vector>>>::first);
        std::size_t idx_theta{};
        if (theta_it == codebook.begin())
            idx_theta = 0;
        else if (theta_it == codebook.end())
            idx_theta = codebook.size() - 1;
        else
        {
            double after = theta_it->first;
            double before = (theta_it - 1)->first;

            std::size_t idx_after = theta_it - codebook.begin();
            std::size_t idx_before = idx_after - 1;

            idx_theta = (after - tilde_theta < tilde_theta - before) ? idx_after : idx_before;
        }
        auto const& radial_cb = codebook.at(idx_theta).second;

        // The radial axis (tilde_r) within a column is non-uniform (grid
        // spacing depends on the column's tilde_theta), so binary-search
        // for the closest ascending entry instead of rounding.
        double const tilde_r = 1 / r_rel;
        auto it = std::ranges::lower_bound(radial_cb, tilde_r, {}, &std::pair<double, cb_vector>::first);
        std::size_t idx_r{};
        if (it == radial_cb.begin())
            idx_r = 0;
        else if (it == radial_cb.end())
            idx_r = radial_cb.size() - 1;
        else
        {
            double after = it->first;
            double before = (it - 1)->first;

            std::size_t idx_after = it - radial_cb.begin();
            std::size_t idx_before = idx_after - 1;

            idx_r = (after - tilde_r < tilde_r - before) ? idx_after : idx_before;
        }
        return radial_cb.at(idx_r).second;
    }

} // namespace antennavision::codebooks