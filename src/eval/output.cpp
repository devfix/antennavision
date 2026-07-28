//
// Created by Tristan Krause on 2026-06-05.
//

#include "eval/output.hpp"
#include <nlohmann/json.hpp>
#include <variant>
#include "NumCpp/Functions/linspace.hpp"
#include "setup/setup.hpp"

namespace eval::output
{
    using std::ranges::transform;

    void directivity_over_polar(std::filesystem::path const& path_json,
        antenna::Antenna const& antenna,
        sweep::Sweep const& sweep_azimuth,
        setup::NumParams const& num_params)
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
    void voltagefield_over_geometry(std::filesystem::path const& path_json,
        ComplexScalarField<T> const& scalar_field,
        geometry::Geometry const& geo,
        sweep::Sweep const& sweep)
    {
        std::ofstream ofs(path_json); // first, acquire file to lock it to our process
        ojson js;
        js["geo"] = geo;
        js["sweep"] = sweep;
        js["num_params"] = scalar_field.num_params;
        auto const [positions, data] = scalar_field.eval_geometry_sweep(geo, sweep);
        js["positions"] = positions;
        js["data"] = data;
        ofs << js.dump(2) << '\n';
    }

    // -----------------------------------------------------------------------------
    // EXPLICIT INSTANTIATION
    // -----------------------------------------------------------------------------
    template void voltagefield_over_geometry<RxVoltageField>(std::filesystem::path const&,
        ComplexScalarField<RxVoltageField> const&,
        geometry::Geometry const&,
        sweep::Sweep const&);

} // namespace eval::output
