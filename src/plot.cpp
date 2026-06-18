//
// Created by Tristan Krause on 2026-06-05.
//

#include "plot.hpp"
#include <NumCpp/Functions/linspace.hpp>
#include <dplot.hpp>

#include "print.hpp"

void plot::plot_directivity_over_theta(std::filesystem::path const &dir_plot, Radiator const &radiator, NdArray const &phis)
{
    std::ostringstream phis_stream;
    phis_stream << std::fixed << std::setprecision(2);
    for (NdArray::index_type k = 0; k < phis.size(); k++)
    {
        phis_stream << std::format("{:.2f}", phis.at(k) / nc::constants::pi);
        if (k < phis.size() - 1) { phis_stream << '_'; }
    }
    std::string name = std::format("{}.{}.{}", __func__, radiator.id, phis_stream.str());
    std::println("Creating plot: {}", name);

    dplot::Figure fig{std::string(name), std::string("test-plot")};
    fig.width = "10cm";
    // fig.background_color = "gray!30";
    fig.legend_setup.enable = true;

    {
        dplot::TickSetup ts;
        ts.enable = true;

        dplot::AxisSetup as;
        as.label = R"($\theta\ / \ \pi$)";
        as.scale = 1.0;
        as.tick = ts;
        as.label_shift = "0.2cm";
        as.grid.major_enable = true;
        as.grid.major_color = "gray!30";
        fig.axes['b'] = as;
    }

    {
        dplot::TickSetup ts;
        ts.enable = true;

        dplot::AxisSetup as;
        as.label = R"($D(\theta,\phi)$)";
        as.scale = 1.0;
        as.tick = ts;
        as.padding = "1cm";
        as.label_shift = "0.2cm";
        as.grid.major_enable = true;
        as.grid.major_color = "gray!30";
        fig.axes['l'] = as;
    }

    {
        dplot::AxisSetup as;
        as.padding = "1cm";
        fig.axes['r'] = as;
    }

    auto const thetas = nc::linspace(0.0, nc::constants::pi, 51);
    NdArray directivities(thetas.shape());
    for (auto const phi : phis)
    {
        std::ranges::transform(thetas, directivities.begin(), [&radiator, phi](double const theta) { return radiator.calc_directivity(theta, phi, 101, 201); });
        fig.add(dplot::Data(dplot::XAxis::B, dplot::YAxis::L, (thetas / nc::constants::pi).toStlVector(), directivities.toStlVector(), std::format("{}\\,=\\,{:.2f}", R"($\phi/\pi$)", phi / nc::constants::pi)));
    }

    fig.export_figure(dir_plot, {dplot::ExportType::PDF});
}

void plot::plot_gain_over_straight(std::filesystem::path const &dir_plot, Radiator const &source, Radiator const &sink, Reference &ref_start, Reference const &ref_stop, double wave_length, char distance_axis)
{

    std::string name = std::format("{}.{}.{}", __func__, source.id, sink.id);
    std::println("Creating plot: {}", name);

    dplot::Figure fig{std::string(name), std::string("test-plot")};
    fig.width = "10cm";
    // fig.background_color = "gray!30";
    fig.legend_setup.enable = true;

    {
        dplot::TickSetup ts;
        ts.enable = true;

        dplot::AxisSetup as;
        as.label = std::format("${} \\ / \\ {}$", distance_axis, R"(\mathrm{m})");
        as.scale = 1.0;
        as.tick = ts;
        as.label_shift = "0.2cm";
        as.grid.major_enable = true;
        as.grid.major_color = "gray!30";
        fig.axes['b'] = as;
    }

    {
        dplot::TickSetup ts;
        ts.enable = true;

        dplot::AxisSetup as;
        as.label = R"($g$)";
        as.scale = 1.0;
        as.tick = ts;
        as.padding = "1cm";
        as.label_shift = "0.2cm";
        as.grid.major_enable = true;
        as.grid.major_color = "gray!30";
        fig.axes['l'] = as;
    }

    {
        dplot::AxisSetup as;
        as.padding = "1cm";
        fig.axes['r'] = as;
    }

    constexpr std::size_t n_points = 101;
    Vec3 const pos_start = ref_start.pos;
    NdArray const rotation_start = ref_start.rotation.toNdArray();
    Vec3 const pos_delta = ref_stop.pos - ref_start.pos;
    NdArray const rotation_delta = ref_stop.rotation.toNdArray() - ref_start.rotation.toNdArray();
    double const length = pos_delta.norm();
    NdArray gians(n_points, 1);
    NdArray distances(n_points, 1);
    double distance = 0;

    double *distance_ptr = &distance;
    switch (distance_axis)
    {
        case 'x': distance_ptr = &ref_start.pos.x; break;
        case 'y': distance_ptr = &ref_start.pos.y; break;
        case 'z': distance_ptr = &ref_start.pos.z; break;
        case 'd':
            break; // already set
        default:
            throw std::runtime_error(std::format("Error in {}: Unknown distance_axis '{}'", __func__, distance_axis));
    }
    for (NdArray::index_type k = 0; k < n_points; k++)
    {
        double const f = static_cast<double>(k) / static_cast<double>(n_points - 1);
        ref_start.pos = pos_start + pos_delta * f;
        ref_start.rotation = rotation_start + rotation_delta * f;
        gians[k] = source.calc_power_gain(sink, wave_length);
        distance = f * length;
        distances[k] = *distance_ptr;
    }
    ref_start.pos = pos_start;
    ref_start.rotation = rotation_start;
    fig.add(dplot::Data(dplot::XAxis::B, dplot::YAxis::L, distances.toStlVector(), gians.toStlVector())); //, std::format("{}\\,=\\,{:.2f}", R"($\phi/\pi$)", phi / nc::constants::pi)));
    fig.export_figure(dir_plot, {dplot::ExportType::PDF});
}

// void plot::gain_over_phase(std::filesystem::path const &dir_plot, NdArray const &phases, std::vector<std::tuple<std::reference_wrapper<const NdArray>, std::string>> const &gains, std::string_view const name,
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
