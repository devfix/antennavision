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
        std::filesystem::path const& path_output,
        antenna::Antenna const& antenna,
        double wavelength,
        sweep::Sweep const& sweep_azimuth,
        setup::SimParams const& sim_params //
    );

    template <typename T>
    void complex_scalarfield_at_wavelength( //
        std::filesystem::path const& path_output,
        setup::task::RxVoltageFieldAtWavelength const& task,
        reference::Reference const& ref,
        ComplexScalarField<T> const& scalar_field,
        geometry::Geometry const& geo,
        sweep::Sweep const& sweep_wavelength //
    );
} // namespace eval::output
