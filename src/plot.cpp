//
// Created by Tristan Krause on 2026-06-05.
//

#include "plot.hpp"
#include <print>
#include <variant>
#include <nlohmann/json.hpp>
#include "NumCpp/Functions/linspace.hpp"
#include "setup.hpp"
#include "simulationerror.hpp"

void plot::plot_directivity_over_polar(Antenna const& antenna, RealArray const& azimuth_angles, math::NumParams const& num_params)
{
    auto& radiator = antenna::cast<Radiator>(antenna);

    std::ostringstream azimuth_angles_stream;
    azimuth_angles_stream << std::fixed << std::setprecision(2);
    for (RealArray::index_type k = 0; k < azimuth_angles.size(); k++)
    {
        azimuth_angles_stream << std::format("{:.2f}", azimuth_angles.at(k) / nc::constants::pi);
        if (k < azimuth_angles.size() - 1) { azimuth_angles_stream << '_'; }
    }
    std::string name = std::format("{}.{}.{}", __func__, antenna::get_id(antenna), azimuth_angles_stream.str());
    std::println("Creating plot: {}", name);

    ojson js;
    js["name"] = name;
    std::vector<ojson> entries;

    auto const polar_angles = nc::linspace(0.0, nc::constants::pi, 51);
    RealArray directivities(polar_angles.shape());
    for (auto const azimuth : azimuth_angles)
    {
        ojson js_entry;
        std::ranges::transform(polar_angles, directivities.begin(),
                               [&](double const theta) { return radiator.calc_directivity_from_spherical(theta, azimuth, num_params); });
        js_entry["azimuth"] = azimuth / nc::constants::pi;
        js_entry["polars"] = (polar_angles / nc::constants::pi).toStlVector();
        js_entry["directivities"] = directivities.toStlVector();
        entries.push_back(std::move(js_entry));
    }
    js["curves"] = entries;

    std::ofstream ofs(std::format("{}.result.json", name));
    ofs << js.dump(2) << '\n';
}

void plot::plot_field_over_geometry(GenericScalarField const& scalar_field, Geometry const& geometry)
{
    if (auto r = std::get_if<geometry::Rectangle>(&geometry); r)
    {
        plot_gain_over_rectangle(scalar_field, *r);
        return;
    }
    if (auto sr = std::get_if<geometry::SphericalRectangle>(&geometry); sr)
    {
        plot_gain_over_spherical_rectangle(scalar_field, *sr);
        return;
    }
    throw SimulationError("Not implemented");
}

void plot::plot_gain_over_line(GenericScalarField const& scalar_field, pos_t const& pos_start, pos_t const& pos_end)
{
    // std::string name = std::format("{}.{}", __func__, scalarfield::get_id(scalar_field));
    // std::println("Creating plot: {}", name);
    //
    // ojson js;
    // js["name"] = name;
    // auto const [positions, gains] = std::visit([&](auto const& field) { return field.eval_line(pos_start, pos_end); }, scalar_field);
    // if (auto ra = std::get_if<RealArray>(&gains); ra) { js["gains"] = ra->toStlVector(); }
    // else if (auto ca = std::get_if<ComplexArray>(&gains); ca) { js["gains"] = ca->toStlVector(); }
    // else
    // {
    //     throw SimulationError("Invalid type of scalar field");
    // }
    // js["positions"] = positions.toStlVector();
    // js["wavelength"] = scalarfield::get_num_params(scalar_field).wavelength;
    //
    // std::ofstream ofs(std::format("{}.result.json", name));
    // ofs << js.dump(2) << '\n';
}

void plot::plot_gain_over_rectangle(GenericScalarField const& scalar_field, geometry::Rectangle const& rectangle)
{
    // std::string name = std::format("{}.{}", __func__, scalarfield::get_id(scalar_field));
    // std::println("Creating plot: {}", name);
    //
    // ojson js;
    // js["name"] = name;
    // auto const [positions, surface_positions, values] = std::visit([&](auto const& field) { return field.eval_plane(rectangle); }, scalar_field);
    // js["positions"] = positions.toStlVector();
    // js["surface_positions"] = surface_positions.toStlVector();
    // std::visit([&js](auto const& v) { js["values"] = v.toStlVector(); }, values);
    // js["num_params"] = scalarfield::get_num_params(scalar_field);
    // js["rectangle"] = rectangle;
    //
    // std::ofstream ofs(std::format("{}.result.json", name));
    // ofs << js.dump(2) << '\n';
}

void plot::plot_gain_over_spherical_rectangle(GenericScalarField const& scalar_field, geometry::SphericalRectangle const& sr)
{
    // std::string name = std::format("{}.{}", __func__, scalarfield::get_id(scalar_field));
    // std::println("Creating plot: {}", name);
    //
    // ojson js;
    // js["name"] = name;
    // auto const [positions, surface_positions, values] = std::visit([&](auto const& field) { return field.eval_sphere(sr); }, scalar_field);
    // js["positions"] = positions.toStlVector();
    // js["surface_positions"] = surface_positions.toStlVector();
    // std::visit([&js](auto const& v) { js["values"] = v.toStlVector(); }, values);
    // js["num_params"] = scalarfield::get_num_params(scalar_field);
    // js["spherical_rectangle"] = sr;
    //
    // std::ofstream ofs(std::format("{}.result.json", name));
    // ofs << js.dump(2) << '\n';
}
