//
// Created by core on 21.06.26.
//

#include "builtin.hpp"
#include <print.hpp>

#include "dplot.hpp"

namespace builtin
{

    void compare_beamwidth(Setup &setup)
    {
        std::filesystem::path const dir_plot = std::filesystem::path(setup.name) / std::filesystem::path("plots");

        std::string name = std::format("builtin.{}", __func__);
        std::println("Creating plot: {}", name);

        dplot::Figure fig{std::string(name), std::string("test-plot")};
        fig.width = "10cm";
        // fig.background_color = "gray!30";
        fig.legend_setup.enable = true;

        {
            dplot::TickSetup ts;
            ts.enable = true;

            dplot::AxisSetup as;
            as.label = std::format("${} \\ / \\ {}$", "z", R"(\mathrm{m})");
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

        Reference &ref_start = setup.get_reference_by_id("ref_rx_start");
        Vec3 const pos_start = ref_start.pos;
        Reference const& ref_stop = setup.get_reference_by_id("ref_rx_stop");

        constexpr std::size_t n_points = 11;
        NdArray const rotation_start = ref_start.rotation.toNdArray();
        Vec3 const pos_delta = ref_stop.pos - ref_start.pos;
        NdArray const rotation_delta = ref_stop.rotation.toNdArray() - ref_start.rotation.toNdArray();
        double const length = pos_delta.norm();
        NdArray gains(n_points, 1);
        NdArray distances(n_points, 1);
        Radiator const& sink = setup.get_radiator_by_id("receiver");
        double const wavelength = setup.variables.at("wavelength");
        double distance = 0;

        double *distance_ptr = &ref_start.pos.z;
        auto const sources = setup.get_radiator_array("ula1");
        for (NdArray::index_type k = 0; k < n_points; k++)
        {
            double const f = static_cast<double>(k) / static_cast<double>(n_points - 1);
            ref_start.pos = pos_start + pos_delta * f;
            ref_start.rotation = rotation_start + rotation_delta * f;

            gains[k] = 0;
            for (Radiator* source : sources)
            {
                gains[k] += source->calc_power_gain(sink, wavelength);
            }
            double const a = setup.get_radiator_by_id("ula1:radiator:0").calc_power_gain(sink, wavelength);
            double const b = setup.get_radiator_by_id("ula1:radiator:1").calc_power_gain(sink, wavelength);
            double const c = setup.get_radiator_by_id("ula1:radiator:2").calc_power_gain(sink, wavelength);
            gains[k] = a+b+c;

            distance = f * length;
            distances[k] = *distance_ptr;
        }
        ref_start.pos = pos_start;
        ref_start.rotation = rotation_start;
        fig.add(dplot::Data(dplot::XAxis::B, dplot::YAxis::L, distances.toStlVector(), gains.toStlVector())); //, std::format("{}\\,=\\,{:.2f}", R"($\phi/\pi$)", phi / nc::constants::pi)));
        fig.export_figure(dir_plot, {dplot::ExportType::PDF});
    }
} // namespace builtin
