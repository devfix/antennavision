//
// Created by Tristan Krause on 2026-06-05.
//

#pragma once

#include "components/antenna.hpp"
#include "eval/rxvoltagefield.hpp"
#include "setup/task.hpp"

namespace eval::output
{
    void directivity_over_polar(setup::task::DirectivityOverPolarAtAzimuth const& task, setup::SimParams const& sim_params);

    namespace voltgain
    {
        template <typename T>
        void points(setup::task::VoltGainOverPoints const& task, ComplexScalarField<T> const& scalar_field);

        template <typename T>
        void geometry(setup::task::VoltGainOverGeometry const& task, ComplexScalarField<T> const& scalar_field);

        template <typename T>
        void geometry_at_wavelength(setup::task::VoltGainOverGeometryAtWavelength const& task, ComplexScalarField<T> const& scalar_field);

        template <typename T>
        void curve_peak_and_cutoff(setup::task::VoltGainPeakAndCutoffs const& task, ComplexScalarField<T> const& scalar_field);
    } // namespace voltgain

} // namespace eval::output
