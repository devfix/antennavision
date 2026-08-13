//
// Created by Tristan Krause on 2026-06-05.
//

#include "eval/output.hpp"
#include <execution>
#include <nlohmann/json.hpp>
#include <variant>
#include "NumCpp/Functions/linspace.hpp"
#include "math/coords.hpp"
#include "setup/setup.hpp"

namespace eval::output
{
    using std::ranges::transform;

    namespace
    {
        Vec3Array calc_positions_spherical(reference::Reference const& ref, Vec3Array const& positions_cartesian)
        {
            Vec3Array positions_spherical(positions_cartesian.shape());
            std::transform( //
                std::execution::par,
                positions_cartesian.begin(),
                positions_cartesian.end(),
                positions_spherical.begin(),
                [&ref](Pos const& pos) -> Pos
                {
                    return math::spherical_from_cartesian_pos<Pos>(ref.local_from_global_pos(pos));
                } //
            );
            return positions_spherical;
        }

        template <AnyJson json_t>
        void save_result(json_t const& js, std::filesystem::path const& path_output, OutputType output_type)
        {
            std::ofstream ofs(path_output, std::ios::binary);
            switch (output_type)
            {
                case OutputType::JSON: ofs << js.dump(2) << '\n'; break;
                case OutputType::BSON: json::to_bson(js, ofs); break;
                case OutputType::CBOR: json::to_cbor(js, ofs); break;
                case OutputType::MSGPACK: json::to_msgpack(js, ofs); break;
                case OutputType::UBJSON: json::to_ubjson(js, ofs); break;
            }
        }
    } // namespace

    std::optional<OutputType> output_type_from_ext(std::string_view ext)
    {
        if (ext == ".json") return OutputType::JSON;
        if (ext == ".bson") return OutputType::BSON;
        if (ext == ".cbor") return OutputType::CBOR;
        if (ext == ".mpk" || ext == ".msgpack") return OutputType::MSGPACK;
        if (ext == ".ubj" || ext == ".ubjson") return OutputType::UBJSON;
        return std::nullopt;
    }

    void directivity_over_polar( //
        std::filesystem::path const& path_output,
        OutputType output_type,
        antenna::Antenna const& ant,
        double wavelength,
        sweep::Sweep const& sweep_azimuth,
        setup::SimParams const& sim_params //
    )
    {
        auto const polar_angles = nc::linspace(0.0, nc::constants::pi, 51);
        auto const azimuths = sweep::get_values(sweep_azimuth);

        std::vector<Complex> coeffs(antenna::size(ant), 1.0);

        std::vector<ojson> entries;
        for (auto const azimuth : azimuths)
        {
            RealArray directivities(polar_angles.shape());
            ojson js_entry;
            transform( //
                polar_angles,
                directivities.begin(),
                [&](double polar)
                {
                    return antenna::calc_directivity_from_spherical(ant, polar, azimuth, wavelength, coeffs, sim_params);
                } //
            );
            js_entry["azimuth"] = azimuth;
            js_entry["polars"] = polar_angles.toStlVector();
            js_entry["directivities"] = directivities.toStlVector();
            entries.push_back(std::move(js_entry));
        }

        ojson js;
        js["sweep"] = sweep_azimuth;
        js["sim_params"] = sim_params;
        js["data"] = entries;
        save_result(js, path_output, output_type);
    }

    template <typename T>
    void complex_scalar_points(std::filesystem::path const& path_output,
        OutputType output_type,
        setup::task::RxVoltage const& task,
        reference::Reference const& ref,
        ComplexScalarField<T> const& scalar_field //
    )
    {
        Vec3Array positions(task.points.size(), 1);
        for (auto k = 0; k < task.points.size(); k++) positions(k,0) = task.points[k];

        auto const gains = scalar_field.eval(positions, task.wavelength);

        ojson js;
        js["sim_params"] = scalar_field.sim_params;
        js["points"] = task.points;
        js["sweep"] = task.wavelength;
        js["task"] = task;
        js["positions"] = ojson();
        js["positions"]["cartesian"] = task.points;
        js["positions"]["spherical"] = calc_positions_spherical(ref, positions).toStlVector();;
        js["gains"] = gains.toStlVector();
        save_result(js, path_output, output_type);
    }

    template <typename T>
    void complex_scalarfield_at_wavelength( //
        std::filesystem::path const& path_output,
        OutputType output_type,
        setup::task::RxVoltageFieldAtWavelength const& task,
        reference::Reference const& ref,
        ComplexScalarField<T> const& scalar_field,
        geometry::Geometry const& geo,
        sweep::Sweep const& sweep_wavelength //
    )
    {
        auto const [positions_cartesian, data] = scalar_field.eval_geometry_sweep(geo, sweep_wavelength, task.n_dim1, task.n_dim2);

        ojson js;
        js["sim_params"] = scalar_field.sim_params;
        js["geo"] = geo;
        js["sweep"] = sweep_wavelength;
        js["task"] = task;
        js["positions"] = ojson();
        js["positions"]["cartesian"] = positions_cartesian;
        js["positions"]["spherical"] = calc_positions_spherical(ref, positions_cartesian);
        js["gains"] = data;
        save_result(js, path_output, output_type);
    }

    // -----------------------------------------------------------------------------
    // EXPLICIT INSTANTIATION
    // -----------------------------------------------------------------------------
    template void complex_scalar_points<RxVoltageField>( //
        std::filesystem::path const&,
        OutputType,
        setup::task::RxVoltage const&,
        reference::Reference const&,
        ComplexScalarField<RxVoltageField> const&//
        );
    template void complex_scalarfield_at_wavelength<RxVoltageField>( //
        std::filesystem::path const&,
        OutputType,
        setup::task::RxVoltageFieldAtWavelength const&,
        reference::Reference const&,
        ComplexScalarField<RxVoltageField> const&,
        geometry::Geometry const&,
        sweep::Sweep const& //
    );

} // namespace eval::output
