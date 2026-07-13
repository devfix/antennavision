//
// Created by Tristan Krause on 2026-06-05.
//

#pragma once

#include <functional>
#include <tuple>
#include <vector>

#include "components/antenna.hpp"
#include "components/radiator.hpp"
#include "components/radiatorarray.hpp"
#include "scalarfield.hpp"
#include "types.hpp"

namespace plot
{
    void plot_directivity_over_polar(std::filesystem::path const& dir_plot, Antenna const& antenna, RealArray const& azimuth_angles);

    void plot_gain_over_straight(std::filesystem::path const& dir_plot, Antenna const& source, Antenna const& sink, Reference& ref_start, Reference const& ref_stop, double wave_length,
                                 char distance_axis);

    void plot_gain_over_plane(std::filesystem::path const& dir_plot, ScalarField const& scalar_field, math::Rectangle const& rectangle, double wavelength, std::uint32_t n_points_axis1,
                                     std::uint32_t n_points_axis2, std::string const& label_axis1, std::string const& label_axis2);

    void gain_over_phase(std::filesystem::path const& dir_plot, RealArray const& phases, std::vector<std::tuple<std::reference_wrapper<const RealArray>, std::string>> const& gains, std::string_view name,
                         std::string_view title);

} // namespace plot
