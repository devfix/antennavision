//
// Created by Tristan Krause on 2026-06-05.
//

#pragma once

#include <functional>
#include <tuple>
#include <vector>

#include "components/antenna.hpp"
#include "scalarfield.hpp"

namespace eval
{
    void plot_directivity_over_polar(Antenna const& antenna, RealArray const& azimuth_angles, math::NumParams const& num_params);

    void plot_field_over_geometry(GenericScalarField const& scalar_field, Geometry const& geometry);

    void plot_gain_over_line(GenericScalarField const& scalar_field, pos_t const& pos_start, pos_t const& pos_end);

    void plot_gain_over_rectangle(GenericScalarField const& scalar_field, geometry::Rectangle const& rectangle);

    void plot_gain_over_spherical_rectangle(GenericScalarField const& scalar_field, geometry::SphericalRectangle const& sr);

    void gain_over_phase(RealArray const& phases, std::vector<std::tuple<std::reference_wrapper<const RealArray>, std::string>> const& gains, std::string_view name,
                         std::string_view title);

} // namespace plot
