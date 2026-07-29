//
// Created by Tristan Krause on 2026-06-05.
//

#include "eval/output.hpp"
#include <execution>
#include <nlohmann/json.hpp>
#include <variant>
#include "NumCpp/Functions/linspace.hpp"
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
                    return math::spherical_from_cartesian<Pos>(ref.local_from_global_pos(pos));
                } //
            );
            return positions_spherical;
        }
    } // namespace

    void directivity_over_polar( //
        std::filesystem::path const& path_json,
        antenna::Antenna const& antenna,
        sweep::Sweep const& sweep_azimuth,
        setup::NumParams const& num_params //
    )
    {
        // at the moment, we only support to calculate the directivity of single radiators
        auto& radiator = antenna::cast<Radiator>(antenna);

        std::ofstream ofs(path_json); // first, acquire file to lock it to our process
        ojson js;
        js["sweep"] = sweep_azimuth;
        js["num_params"] = num_params;

        auto const polar_angles = nc::linspace(0.0, nc::constants::pi, 51);
        auto const azimuths = sweep::get_values(sweep_azimuth);

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
                    return radiator.calc_directivity_from_spherical(polar, azimuth, num_params);
                } //
            );
            js_entry["azimuth"] = azimuth;
            js_entry["polars"] = polar_angles.toStlVector();
            js_entry["directivities"] = directivities.toStlVector();
            entries.push_back(std::move(js_entry));
        }
        js["data"] = entries;
        ofs << js.dump(2) << '\n';
    }

    template <typename T>
    void complex_scalarfield_at_wavelength( //
        std::filesystem::path const& path_json,
        setup::task::RxVoltageFieldAtWavelength const& task,
        reference::Reference const& ref,
        ComplexScalarField<T> const& scalar_field,
        geometry::Geometry const& geo,
        sweep::Sweep const& sweep_wavelength //
    )
    {
        std::ofstream ofs(path_json); // first, acquire file to lock it to our process
        ojson js;
        js["num_params"] = scalar_field.num_params;
        js["geo"] = geo;
        js["sweep"] = sweep_wavelength;
        js["task"] = task;
        auto const [positions_cartesian, data] = scalar_field.eval_geometry_sweep(geo, task.n_dim1, task.n_dim2, sweep_wavelength);
        js["positions"] = ojson();
        js["positions"]["cartesian"] = positions_cartesian;
        js["positions"]["spherical"] = calc_positions_spherical(ref, positions_cartesian);
        js["data"] = data;
        ofs << js.dump(2) << '\n';
    }

    // -----------------------------------------------------------------------------
    // EXPLICIT INSTANTIATION
    // -----------------------------------------------------------------------------
    template void complex_scalarfield_at_wavelength<RxVoltageField>( //
        std::filesystem::path const&,
        setup::task::RxVoltageFieldAtWavelength const&,
        reference::Reference const&,
        ComplexScalarField<RxVoltageField> const&,
        geometry::Geometry const&,
        sweep::Sweep const& //
    );

} // namespace eval::output
