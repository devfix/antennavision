//
// Created by Tristan Krause on 2026-06-05.
//

#include "eval/output.hpp"
#include <execution>
#include <nlohmann/json.hpp>
#include <variant>
#include "NumCpp/Functions/linspace.hpp"
#include "math/coords.hpp"

namespace eval::output
{
    using setup::task::OutputType;
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
                    return math::spherical_from_cartesian_pos_impl<Pos>(ref.local_from_global_pos(pos));
                } //
            );
            return positions_spherical;
        }

        template <AnyJson JsonType>
        void save_result(JsonType const& js, setup::task::Task const& task)
        {
            std::ofstream ofs(setup::task::get_output_path(task), std::ios::binary);
            switch (setup::task::get_output_type(task))
            {
                case OutputType::JSON: ofs << js.dump(2) << '\n'; break;
                case OutputType::BSON: json::to_bson(js, ofs); break;
                case OutputType::CBOR: json::to_cbor(js, ofs); break;
                case OutputType::MSGPACK: json::to_msgpack(js, ofs); break;
                case OutputType::UBJSON: json::to_ubjson(js, ofs); break;
            }
        }
    } // namespace

    void directivity_over_polar(setup::task::DirectivityOverPolarAtAzimuth const& task, setup::SimParams const& sim_params)
    {
        auto const polar_angles = nc::linspace(0.0, nc::constants::pi, 51);
        auto const azimuths = sweep::get_values(task.sweep_azimuth);

        std::vector<Complex> coeffs(components::antenna::size(task.tx), 1.0);

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
                    return components::antenna::calc_directivity_from_spherical(task.tx, polar, azimuth, task.wavelength, coeffs, sim_params);
                } //
            );
            js_entry["azimuth"] = azimuth;
            js_entry["polars"] = polar_angles.toStlVector();
            js_entry["directivities"] = directivities.toStlVector();
            entries.push_back(std::move(js_entry));
        }

        ojson js;
        js["sweep"] = task.sweep_azimuth;
        js["sim_params"] = sim_params;
        js["data"] = entries;
        save_result(js, task);
    }

    namespace voltgain
    {
        template <typename T>
        void points(setup::task::VoltGainOverPoints const& task, ComplexScalarField<T> const& scalar_field)
        {
            Vec3Array positions(task.points.size(), 1);
            for (auto k = 0; k < task.points.size(); k++) positions(k, 0) = task.points[k];

            auto const gains = scalar_field.eval(positions, task.wavelength);

            ojson js;
            js["sim_params"] = scalar_field.sim_params;
            js["task"] = task;
            js["positions"] = ojson();
            js["positions"]["cartesian"] = task.points;
            js["positions"]["spherical"] = calc_positions_spherical(task.ref, positions).toStlVector();
            js["gains"] = gains.toStlVector();
            save_result(js, task);
        }

        template <typename T>
        void geometry(setup::task::VoltGainOverGeometry const& task, ComplexScalarField<T> const& scalar_field)
        {
            auto const [positions_cartesian, data] = scalar_field.eval_geometry(task.geo, task.wavelength, task.n_dim1, task.n_dim2);

            ojson js;
            js["sim_params"] = scalar_field.sim_params;
            js["geo"] = task.geo;
            js["task"] = task;
            js["positions"] = ojson();
            js["positions"]["cartesian"] = positions_cartesian;
            js["positions"]["spherical"] = calc_positions_spherical(task.ref, positions_cartesian);
            js["gains"] = data;
            save_result(js, task);
        }

        template <typename T>
        void geometry_at_wavelength(setup::task::VoltGainOverGeometryAtWavelength const& task, ComplexScalarField<T> const& scalar_field)
        {
            auto const [positions_cartesian, data] = scalar_field.eval_geometry_sweep(task.geo, task.sweep_wavelength, task.n_dim1, task.n_dim2);

            ojson js;
            js["sim_params"] = scalar_field.sim_params;
            js["geo"] = task.geo;
            js["sweep"] = task.sweep_wavelength;
            js["task"] = task;
            js["positions"] = ojson();
            js["positions"]["cartesian"] = positions_cartesian;
            js["positions"]["spherical"] = calc_positions_spherical(task.ref, positions_cartesian);
            js["gains"] = data;
            save_result(js, task);
        }

        template <typename T>
        void curve_peak_and_cutoff(setup::task::VoltGainPeakAndCutoffs const& task, ComplexScalarField<T> const& scalar_field)
        {
            auto const curve_peak_span = scalar_field.find_curve_peak_and_cutoffs(task.curve, task.wavelength, task.ratio, task.n_scan);

            ojson js;
            js["sim_params"] = scalar_field.sim_params;
            js["curve"] = task.curve;
            js["task"] = task;
            js["peak"] = curve_peak_span.peak;
            js["pos_peak"] = curve_peak_span.pos_peak;
            js["pos_left"] = curve_peak_span.pos_left;
            js["pos_right"] = curve_peak_span.pos_right;
            save_result(js, task);
        }
    } // namespace voltgain

    // -----------------------------------------------------------------------------
    // EXPLICIT INSTANTIATION
    // -----------------------------------------------------------------------------
    template void voltgain::points<RxVoltageField>(setup::task::VoltGainOverPoints const& task, ComplexScalarField<RxVoltageField> const& scalar_field);
    template void voltgain::geometry<RxVoltageField>(setup::task::VoltGainOverGeometry const& task, ComplexScalarField<RxVoltageField> const& scalar_field);
    template void voltgain::geometry_at_wavelength<RxVoltageField>(setup::task::VoltGainOverGeometryAtWavelength const& task,
        ComplexScalarField<RxVoltageField> const& scalar_field);
    template void voltgain::curve_peak_and_cutoff<RxVoltageField>(setup::task::VoltGainPeakAndCutoffs const& task,
        ComplexScalarField<RxVoltageField> const& scalar_field);

} // namespace eval::output
