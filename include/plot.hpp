//
// Created by Tristan Krause on 2026-06-05.
//

#pragma once

#include <functional>
#include <tuple>
#include <vector>
#include "components/radiator.hpp"
#include "types.hpp"

namespace plot
{
    void plot_directivity_over_theta(path const& dir_plot, Radiator const& radiator, NdArray const& phis);

    void gain_over_phase(path const& dir_plot, NdArray const& phases, std::vector<std::tuple<std::reference_wrapper<const NdArray>, std::string>> const& gains, std::string_view name, std::string_view title);

} // namespace plot