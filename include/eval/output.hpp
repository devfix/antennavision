//
// Created by Tristan Krause on 2026-06-05.
//

#pragma once

#include "components/antenna.hpp"
#include "eval/rxvoltagefield.hpp"
#include "setup/task.hpp"

namespace eval::output
{
    void directivity_over_polar( //
        std::filesystem::path const& path_json,
        antenna::Antenna const& antenna,
        sweep::Sweep const& sweep_azimuth,
        setup::NumParams const& num_params //
    );

    template <typename T>
    void complex_scalarfield_at_wavelength( //
        std::filesystem::path const& path_json,
        setup::task::RxVoltageFieldAtWavelength const& task,
        reference::Reference const& ref,
        ComplexScalarField<T> const& scalar_field,
        geometry::Geometry const& geo,
        sweep::Sweep const& sweep_wavelength //
    );
} // namespace eval::output
