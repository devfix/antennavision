//
// Created by Tristan Krause on 2026-06-05.
//

#include "plot.hpp"
#include <dplot.hpp>
#include "print.hpp"

void plot::plot_directivity_over_theta(path const &dir_plot, Radiator const &radiator, NdArray const &phis)
{
    std::ostringstream phis_stream;
    phis_stream << std::fixed << std::setprecision(2);
    for (NdArray::index_type k = 0; k < phis.size(); k++)
    {
        phis_stream << std::format("{:.2f}", phis.at(k) / PI);
        if (k < phis.size() - 1) { phis_stream << '_'; }
    }
    std::string name = std::format("{}.{}.{}", __func__, radiator.id, phis_stream.str());
    std::println("creating plot: {}", name);

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

    auto const thetas = nc::linspace(0.0, PI, 51);
    NdArray directivities(thetas.shape());
    for (auto const phi : phis)
    {
        std::ranges::transform(thetas, directivities.begin(), [&radiator, phi](double const theta) { return radiator.calc_directivity(theta, phi, 101, 201); });
        fig.add(dplot::Data(dplot::XAxis::B, dplot::YAxis::L, thetas / PI, directivities, std::format("{}\\,=\\,{:.2f}", R"($\phi/\pi$)", phi / PI)));
    }

    fig.export_figure(dir_plot, {dplot::ExportType::PDF});
}

void plot::plot_gain_over_straight(path const &dir_plot, Radiator const &source, Radiator const &sink, Reference &ref_start, Reference const &ref_stop, double wave_length)
{
    std::string name = std::format("{}.{}.{}", __func__, source.id, sink.id);
    std::println("creating plot: {}", name);

    dplot::Figure fig{std::string(name), std::string("test-plot")};
    fig.width = "10cm";
    // fig.background_color = "gray!30";
    fig.legend_setup.enable = true;

    {
        dplot::TickSetup ts;
        ts.enable = true;

        dplot::AxisSetup as;
        as.label = R"($d\ / \ \mathrm{m}$)";
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
    Vec3 const delta = ref_stop.translation - ref_start.translation;
    double const length = delta.norm();
    Vec3 const start = ref_start.translation;
    NdArray gain(n_points);
    NdArray distance(n_points);
    for (std::size_t k = 0; k < n_points; k++)
    {
        double const f = static_cast<double>(k) / static_cast<double>(n_points - 1);
        ref_start.translation = start + f * delta;
        gain[k] = source.calc_power_gain(sink, wave_length);
        distance[k] = f * length;

    }
    fig.add(dplot::Data(dplot::XAxis::B, dplot::YAxis::L, distance, gain)); //, std::format("{}\\,=\\,{:.2f}", R"($\phi/\pi$)", phi / PI)));
    fig.export_figure(dir_plot, {dplot::ExportType::PDF});
}

void plot::gain_over_phase(path const &dir_plot, NdArray const &phases, std::vector<std::tuple<std::reference_wrapper<const NdArray>, std::string>> const &gains, std::string_view const name,
                           std::string_view const title)
{
    dplot::Figure fig{std::string(name), std::string(title)};

    fig.background_color = "gray!30";
    fig.legend_setup.enable = false;

    dplot::TickSetup ts;
    ts.enable = true;

    dplot::AxisSetup ax_b;
    ax_b.label = "x";
    ax_b.scale = 1.0;
    ax_b.tick = ts;
    fig.axes['b'] = ax_b;

    dplot::AxisSetup ax_l;
    ax_l.label = "y";
    ax_l.scale = 1.0;
    ax_l.tick = ts;
    fig.axes['l'] = ax_l;

    for (auto const &[gain, label] : gains) { fig.add(dplot::Data(dplot::XAxis::B, dplot::YAxis::L, phases, gain, label)); }

    // (Alternatively, you can use the plot helper function)
    // fig.plot(XAxis::B, YAxis::L, {-2, -1, 0, 1, 2}, {5, 1, 0, 1, 5});

    // 5. Export
    // C++ takes a std::vector of ExportTypes, so we wrap it in {}
    fig.export_figure(dir_plot, {dplot::ExportType::PDF});
}
