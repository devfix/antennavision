//
// Created by Tristan Krause on 2026-06-05.
//

#pragma once

#include "components/antenna.hpp"
#include "eval/rxvoltagefield.hpp"
#include "setup/task.hpp"

namespace eval::output
{
    enum struct OutputType
    {
        JSON,
        BSON,
        CBOR,
        MSGPACK,
        UBJSON
    };

    std::optional<OutputType> output_type_from_ext(std::string_view ext);

    void directivity_over_polar( //
        std::filesystem::path const& path_output,
        OutputType output_type,
        antenna::Antenna const& ant,
        double wavelength,
        sweep::Sweep const& sweep_azimuth,
        setup::SimParams const& sim_params //
    );

    namespace voltgain
    {
        template <typename T>
        void points( //
            std::filesystem::path const& path_output,
            OutputType output_type,
            setup::task::VoltGainOverPoints const& task,
            reference::Reference const& ref,
            ComplexScalarField<T> const& scalar_field //
            );

        template <typename T>
        void geometry( //
            std::filesystem::path const& path_output,
            OutputType output_type,
            setup::task::VoltGainOverGeometry const& task,
            reference::Reference const& ref,
            ComplexScalarField<T> const& scalar_field,
            geometry::Geometry const& geo//
        );

        template <typename T>
        void geometry_at_wavelength( //
            std::filesystem::path const& path_output,
            OutputType output_type,
            setup::task::VoltGainOverGeometryAtWavelength const& task,
            reference::Reference const& ref,
            ComplexScalarField<T> const& scalar_field,
            geometry::Geometry const& geo,
            sweep::Sweep const& sweep_wavelength //
        );

        template <typename T>
        void curve_peak_and_cutoff( //
            std::filesystem::path const& path_output,
            OutputType output_type,
            setup::task::VoltGainPeakAndCutoffs const& task,
            reference::Reference const& ref,
            ComplexScalarField<T> const& scalar_field,
            geometry::Curve const& curve //
        );
    } // namespace voltgain

} // namespace eval::output
