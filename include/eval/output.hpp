//
// Created by Tristan Krause on 2026-06-05.
//

#pragma once

#include "eval/rxvoltagefield.hpp"
#include "components/antenna.hpp"

namespace eval::output
{
    void directivity_over_polar( //
        std::filesystem::path const& path_json,
        antenna::Antenna const& antenna,
        sweep::Sweep const& sweep_azimuth,
        setup::NumParams const& num_params //
    );

    template <typename T>
    void voltagefield_over_geometry( //
        std::filesystem::path const& path_json,
        ComplexScalarField<T> const& scalar_field,
        geometry::Geometry const& geo,
        sweep::Sweep const& sweep //
    );
} // namespace eval::output
