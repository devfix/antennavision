//
// Created by core on 21.06.26.
//

#include <vector>
#include <print>
#include <nlohmann/json.hpp>
#include "builtin.hpp"
#include "math.hpp"

namespace builtin
{
    namespace
    {
        ojson const js_template = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "ula-beamwidth"
  },
  "variables": {
    "system_wavelength": 0.1,
    "wavelength": "system_wavelength",
    "distance": 10000,
    "dipole_length": "0.5 * wavelength"
  },
  "references": [
    {
      "id": "ref_rx",
      "origin": ""
    }
  ],
  "antennas": [
    {
      "id": "tx",
      "type": "ULA",
      "ref": "",
      "spacing": "wavelength * 0.5",
      "size": 1,
      "radiator": {
        "type": "StandingWaveDipole",
        "dipole_length": "dipole_length"
      }
    },
    {
      "id": "rx",
      "ref": "ref_rx",
      "type": "StandingWaveDipole",
      "dipole_length": "dipole_length"
    }
  ]
}
)JSON");
    }

    BUILTIN_FUNCTION(t00_ula_beamwidth, Setup& setup_task)
    {
        std::filesystem::path const dir_plot = std::filesystem::path(setup_task.name);

        std::string name = std::format("builtin.{}", __func__);
        std::println("Creating plot: {}", name);

        json js;
        js["name"] = name;

        std::vector<std::uint32_t> ns_elements;
        std::vector<double> beamwidths_axial;
        std::vector<double> beamwidths_lateral;
        std::uint32_t n_elements_min = static_cast<std::uint32_t>(std::round(setup_task.variables.at("n_elements_min")));
        std::uint32_t n_elements_max = static_cast<std::uint32_t>(std::round(setup_task.variables.at("n_elements_max")));
        for (std::uint32_t n_elements = n_elements_min; n_elements <= n_elements_max; n_elements++)
        {
            std::println("Calculating beamwidth for n={}", n_elements);
            ns_elements.push_back(n_elements);
            auto js_configured = js_template;
            js_configured.at("antennas").at(0).at("size") = n_elements;

            math::NumParams num_params;
            num_params.n_linear1 = 201;
            {
                json json_rot;
                json_rot["roll"] = 0.0;
                json_rot["pitch"] = 0.5;
                json_rot["yaw"] = 0.0;
                js_configured.at("antennas").at(0)["rot"] = json_rot;
                auto const setup = Setup::from_json(js_configured);
                num_params.wavelength = setup->variables.at("wavelength");
                auto const distance = setup->variables.at("distance");
                auto& tx = setup->get_antenna("tx");
                auto& rx = setup->get_antenna("rx");
                auto voltage_field = antenna::get_voltage_field(tx, rx, num_params);
                auto circle = geometry::CircleArc(POS_ZERO, pos_t(0, 0, 1), pos_t(0, 1, 0), distance, 0.5*pi);

                auto [pos_beam, beamwidth_axial] = voltage_field.calc_beamwidth(circle, sqrt2_2);
                beamwidths_axial.push_back(beamwidth_axial);
                if (n_elements == n_elements_max) { setup->export_to_three(".", "axial"); }
            }
            {
                json json_rot;
                json_rot["roll"] = 0.0;
                json_rot["pitch"] = 0.0;
                json_rot["yaw"] = 0.0;
                js_configured.at("antennas").at(0)["rot"] = json_rot;
                auto const setup = Setup::from_json(js_configured);
                num_params.wavelength = setup->variables.at("wavelength");
                auto const distance = setup->variables.at("distance");
                auto& tx = setup->get_antenna("tx");
                auto& rx = setup->get_antenna("rx");
                auto voltage_field = antenna::get_voltage_field(tx, rx, num_params);
                auto circle = geometry::CircleArc(POS_ZERO, pos_t(0, 0, 1), pos_t(0, 1, 0), distance, 0.5*pi);

                auto [pos_beam, beamwidth_lateral] = voltage_field.calc_beamwidth(circle, sqrt2_2);
                beamwidths_lateral.push_back(beamwidth_lateral);
                if (n_elements == n_elements_max) { setup->export_to_three(".", "lateral"); }
            }
        }

        js["ns_elements"] = ns_elements;
        js["beamwidths_axial"] = beamwidths_axial;
        js["beamwidths_lateral"] = beamwidths_lateral;
        std::println("{}", std::filesystem::current_path().string());
        std::println("Saving {}/{}.json", dir_plot.c_str(), name);

        std::ofstream ofs(std::format("{}.result.json", name));
        ofs << js.dump(2) << '\n';
    }
} // namespace builtin
