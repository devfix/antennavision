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
        JSON, BSON, CBOR, MSGPACK, UBJSON
    };

    std::optional<OutputType> output_type_from_ext (std::string_view ext);

    void directivity_over_polar( //
        std::filesystem::path const& path_output,
        OutputType output_type,
        antenna::Antenna const& ant,
        double wavelength,
        sweep::Sweep const& sweep_azimuth,
        setup::SimParams const& sim_params //
        );

    template <typename T>
    void complex_scalar_points( //
        std::filesystem::path const& path_output,
        OutputType output_type,
        setup::task::RxVoltage const& task,
        reference::Reference const& ref,
        ComplexScalarField<T> const& scalar_field//
    );

    template <typename T>
    void complex_scalarfield_at_wavelength( //
        std::filesystem::path const& path_output,
        OutputType output_type,
        setup::task::RxVoltageFieldAtWavelength const& task,
        reference::Reference const& ref,
        ComplexScalarField<T> const& scalar_field,
        geometry::Geometry const& geo,
        sweep::Sweep const& sweep_wavelength //
    );
} // namespace eval::output
