//
// Created by Tristan Krause on 2026-06-05.
//

#pragma once

#include <functional>
#include <tuple>
#include <vector>
#include "components/radiator.hpp"
#include "components/radiatorarray.hpp"
#include "types.hpp"

namespace plot
{
    void plot_directivity_over_polar(std::filesystem::path const& dir_plot, Radiator const& radiator, NdArray const& azimuth_angles);

    void plot_gain_over_straight(std::filesystem::path const& dir_plot, Radiator const& source, Radiator const& sink, Reference& ref_start, Reference const& ref_stop, double wave_length,
                                 char distance_axis);

    void plot_gain_over_plane(std::filesystem::path const& dir_plot, radiator_t const& source, Radiator const& sink, Reference& ref_zero, Reference const& ref_axis1_max, Reference const& ref_axis2_max,
                              double wavelength, std::uint32_t n_points_axis1, std::uint32_t n_points_axis2, std::string const& label_axis1, std::string const& label_axis2);

    void gain_over_phase(std::filesystem::path const& dir_plot, NdArray const& phases, std::vector<std::tuple<std::reference_wrapper<const NdArray>, std::string>> const& gains, std::string_view name,
                         std::string_view title);

} // namespace plot
