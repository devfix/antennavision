//
// Created by Tristan Krause on 2026-06-05.
//

#include "plot.hpp"
#include <NumCpp/Functions/linspace.hpp>
#include <nlohmann/json.hpp>

#include "print.hpp"
#include "setup.hpp"

void plot::plot_directivity_over_polar(std::filesystem::path const& dir_plot, Radiator const& radiator, NdArray const& azimuth_angles)
{
    std::ostringstream azimuth_angles_stream;
    azimuth_angles_stream << std::fixed << std::setprecision(2);
    for (NdArray::index_type k = 0; k < azimuth_angles.size(); k++)
    {
        azimuth_angles_stream << std::format("{:.2f}", azimuth_angles.at(k) / nc::constants::pi);
        if (k < azimuth_angles.size() - 1) { azimuth_angles_stream << '_'; }
    }
    std::string name = std::format("{}.{}.{}", __func__, radiator.id, azimuth_angles_stream.str());
    std::println("Creating plot: {}", name);

    // dplot::Figure fig{std::string(name), std::string("test-plot")};
    // fig.width = "10cm";
    // // fig.background_color = "gray!30";
    // fig.legend_setup.enable = true;
    //
    // {
    //     dplot::TickSetup ts;
    //     ts.enable = true;
    //
    //     dplot::AxisSetup as;
    //     as.label = R"($\theta\ / \ \pi$)";
    //     as.scale = 1.0;
    //     as.tick = ts;
    //     as.label_shift = "0.2cm";
    //     as.grid.major_enable = true;
    //     as.grid.major_color = "gray!30";
    //     fig.axes['b'] = as;
    // }
    //
    // {
    //     dplot::TickSetup ts;
    //     ts.enable = true;
    //
    //     dplot::AxisSetup as;
    //     as.label = R"($D(\theta,\phi)$)";
    //     as.scale = 1.0;
    //     as.tick = ts;
    //     as.padding = "1cm";
    //     as.label_shift = "0.2cm";
    //     as.grid.major_enable = true;
    //     as.grid.major_color = "gray!30";
    //     fig.axes['l'] = as;
    // }
    //
    // {
    //     dplot::AxisSetup as;
    //     as.padding = "1cm";
    //     fig.axes['r'] = as;
    // }

    json js;
    js["name"] = name;
    std::vector<json> entries;

    double const wavelength = 0.1;

    auto const polar_angles = nc::linspace(0.0, nc::constants::pi, 51);
    NdArray directivities(polar_angles.shape());
    for (auto const azimuth : azimuth_angles)
    {
        json js_entry;
        std::ranges::transform(polar_angles, directivities.begin(), [&radiator, azimuth, wavelength](double const theta) { return radiator.calc_directivity_from_spherical(theta, azimuth, wavelength, {}); });
        js_entry["azimuth"] = azimuth / nc::constants::pi;
        js_entry["polars"] = (polar_angles / nc::constants::pi).toStlVector();
        js_entry["directivities"] = directivities.toStlVector();
        entries.push_back(std::move(js_entry));
    }
    js["curves"] = entries;

    std::ofstream ofs(std::format("{}/{}.json", dir_plot.c_str(), name));
    ofs << js.dump(2) << '\n';

    // fig.export_figure(dir_plot, {dplot::ExportType::PDF});
}

void plot::plot_gain_over_straight(std::filesystem::path const& dir_plot, Radiator const& source, Radiator const& sink, Reference& ref_start, Reference const& ref_stop, double wave_length,
                                   char distance_axis)
{
    std::string name = std::format("{}.{}.{}.{}", __func__, source.id, sink.id, distance_axis);
    std::println("Creating plot: {}", name);

    // dplot::Figure fig{std::string(name), std::string("test-plot")};
    // fig.width = "10cm";
    // // fig.background_color = "gray!30";
    // fig.legend_setup.enable = true;
    //
    // {
    //     dplot::TickSetup ts;
    //     ts.enable = true;
    //
    //     dplot::AxisSetup as;
    //     as.label = std::format("${} \\ / \\ {}$", distance_axis, R"(\mathrm{m})");
    //     as.scale = 1.0;
    //     as.tick = ts;
    //     as.label_shift = "0.2cm";
    //     as.grid.major_enable = true;
    //     as.grid.major_color = "gray!30";
    //     fig.axes['b'] = as;
    // }
    //
    // {
    //     dplot::TickSetup ts;
    //     ts.enable = true;
    //
    //     dplot::AxisSetup as;
    //     as.label = R"($g$)";
    //     as.scale = 1.0;
    //     as.tick = ts;
    //     as.padding = "1cm";
    //     as.label_shift = "0.2cm";
    //     as.grid.major_enable = true;
    //     as.grid.major_color = "gray!30";
    //     fig.axes['l'] = as;
    // }
    //
    // {
    //     dplot::AxisSetup as;
    //     as.padding = "1cm";
    //     fig.axes['r'] = as;
    // }

    json js;
    js["name"] = name;

    constexpr std::size_t n_points = 101;
    Vec3 const pos_start = ref_start.pos;
    NdArray const rotation_start = ref_start.rotation.toNdArray();
    Vec3 const pos_delta = ref_stop.pos - ref_start.pos;
    NdArray const rotation_delta = ref_stop.rotation.toNdArray() - ref_start.rotation.toNdArray();
    double const length = pos_delta.norm();
    NdArray gains(n_points, 1);
    NdArray distances(n_points, 1);
    double distance = 0;

    double* distance_ptr = &distance;
    switch (distance_axis)
    {
        case 'x': distance_ptr = &ref_start.pos.x; break;
        case 'y': distance_ptr = &ref_start.pos.y; break;
        case 'z': distance_ptr = &ref_start.pos.z; break;
        case 'd': break; // already set
        default: throw std::runtime_error(std::format("Error in {}: Unknown distance_axis '{}'", __func__, distance_axis));
    }
    for (NdArray::index_type k = 0; k < n_points; k++)
    {
        double const f = static_cast<double>(k) / static_cast<double>(n_points - 1);
        ref_start.pos = pos_start + pos_delta * f;
        ref_start.rotation = rotation_start + rotation_delta * f;
        gains[k] = Setup::calc_power_gain(source, sink, wave_length, {});
        distance = f * length;
        distances[k] = *distance_ptr;
    }
    ref_start.pos = pos_start;
    ref_start.rotation = rotation_start;

    js["distance_axis"] = std::string{distance_axis};
    js["distances"] = distances.toStlVector();
    js["gains"] = gains.toStlVector();
    // fig.add(dplot::Data(dplot::XAxis::B, dplot::YAxis::L, distances.toStlVector(), gains.toStlVector())); //, std::format("{}\\,=\\,{:.2f}", R"($\phi/\pi$)", phi / nc::constants::pi)));
    // fig.export_figure(dir_plot, {dplot::ExportType::PDF});

    std::ofstream ofs(std::format("{}/{}.json", dir_plot.c_str(), name));
    ofs << js.dump(2) << '\n';
}

// void plot::gain_over_phase(std::filesystem::path const &dir_plot, NdArray const &phases, std::vector<std::tuple<std::reference_wrapper<const NdArray>, std::string>> const &gains, std::string_view
// const name,
//                            std::string_view const title)
// {
//     dplot::Figure fig{std::string(name), std::string(title)};
//
//     fig.background_color = "gray!30";
//     fig.legend_setup.enable = false;
//
//     dplot::TickSetup ts;
//     ts.enable = true;
//
//     dplot::AxisSetup ax_b;
//     ax_b.label = "x";
//     ax_b.scale = 1.0;
//     ax_b.tick = ts;
//     fig.axes['b'] = ax_b;
//
//     dplot::AxisSetup ax_l;
//     ax_l.label = "y";
//     ax_l.scale = 1.0;
//     ax_l.tick = ts;
//     fig.axes['l'] = ax_l;
//
//     for (auto const &[gain, label] : gains)
//     {
//         NdArray copy(gain[0]);
//         fig.add(dplot::Data(dplot::XAxis::B, dplot::YAxis::L, phases.toStlVector(), copy.toStlVector(), label));
//     }
//
//     // (Alternatively, you can use the plot helper function)
//     // fig.plot(XAxis::B, YAxis::L, {-2, -1, 0, 1, 2}, {5, 1, 0, 1, 5});
//
//     // 5. Export
//     // C++ takes a std::vector of ExportTypes, so we wrap it in {}
//     fig.export_figure(dir_plot, {dplot::ExportType::PDF});
// }
