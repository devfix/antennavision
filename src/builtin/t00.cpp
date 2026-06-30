//
// Created by core on 21.06.26.
//

#include "../../include/builtin/t00.hpp"
#include <nlohmann/json.hpp>
#include <print.hpp>

#include "NumCpp/Functions/linspace.hpp"
#include "math.hpp"

namespace builtin
{

    void t00_compare_beamwidth(Setup& setup)
    {
        std::filesystem::path const dir_plot = std::filesystem::path(setup.name);

        std::string name = std::format("builtin.{}", __func__);
        std::println("Creating plot: {}", name);

        json js;
        js["name"] = name;
        std::vector<json> entries;

        Reference& ref_start = setup.get_reference_by_id("ref_rx_start");
        Vec3 const pos_start = ref_start.pos;
        Reference const& ref_stop = setup.get_reference_by_id("ref_rx_stop");

        constexpr std::size_t n_points = 101;
        NdArray const rotation_start = ref_start.rotation.toNdArray();
        Vec3 const pos_delta = ref_stop.pos - ref_start.pos;
        NdArray const rotation_delta = ref_stop.rotation.toNdArray() - ref_start.rotation.toNdArray();
        double const length = pos_delta.norm();

        std::vector<double> gains(n_points, 0.0);
        std::vector<double> distances(n_points, 0.0);

        Radiator const& sink = setup.get_radiator_by_id("receiver");
        double const wavelength = setup.variables.at("wavelength");
        double distance = 0;

        double* distance_ptr = &ref_start.pos.z;
        auto const sources = setup.get_radiator_array("ula1");
        for (NdArray::index_type k = 0; k < n_points; k++)
        {
            double const f = static_cast<double>(k) / static_cast<double>(n_points - 1);
            ref_start.pos = pos_start + pos_delta * f;
            ref_start.rotation = rotation_start + rotation_delta * f;

            gains[k] = 0;
            complex_t gain = 0;
            for (Radiator* source : sources) { gain += Radiator::calc_voltage_gain(*source, sink, wavelength, {}); }
            gains[k] = math::square(std::abs(gain));

            distance = f * length;
            distances[k] = *distance_ptr;
        }
        ref_start.pos = pos_start;
        ref_start.rotation = rotation_start;

        js["distances"] = distances;
        js["gains"] = gains;

        std::ofstream ofs(std::format("{}/{}.json", dir_plot.c_str(), name));
        ofs << js.dump(2) << '\n';
    }
} // namespace builtin
