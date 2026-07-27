//
// Created by Tristan Krause on 2026-06-05.
//

#pragma once

#include "components/antenna.hpp"
#include "voltagefield.hpp"

namespace eval::output
{
    void directivity_over_polar(antenna::Antenna const& antenna, RealArray const& azimuth_angles, setup::NumParams const& num_params);

    template<typename T>
    void voltagefield_over_geometry(ComplexScalarField<T> const& scalar_field, geometry::Geometry const& geo, sweep::Sweep const& sweep);

    //
    // void gain_over_phase(RealArray const& phases, std::vector<std::tuple<std::reference_wrapper<const RealArray>, std::string>> const& gains,
    // std::string_view name,
    //                      std::string_view title);

} // namespace eval::output
