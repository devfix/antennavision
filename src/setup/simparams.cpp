//
// Created by Tristan Krause on 2026-07-24.
//

#include "setup/simparams.hpp"
#include <nlohmann/json.hpp>
#include "serialization.hpp"

namespace setup
{
    template <AnyJson JsonType>
    void to_json(JsonType& js, SimParams const& sim_params)
    {
        js = JsonType{
            {"system_wavelength", sim_params.system_wavelength},
            {"enable_path_loss", sim_params.enable_path_loss},
            {"n_polar", sim_params.n_polar},
            {"n_azimuth", sim_params.n_azimuth},
            {"n_linear1", sim_params.n_linear1},
            {"n_linear2", sim_params.n_linear2},
            {"xtol_rel", sim_params.xtol_rel},
            {"ftol_rel", sim_params.ftol_rel} //
        };
    }

    template <AnyJson JsonType>
    void from_json(JsonType const& js, SimParams& sim_params)
    {
        serialization::assert_structure(js,
            "NumParams",
            {
                {"system_wavelength", json::value_t::number_float},
            },
            {
                {"n_polar", json::value_t::number_unsigned},
                {"n_azimuth", json::value_t::number_unsigned},
                {"n_linear1", json::value_t::number_unsigned},
                {"n_linear2", json::value_t::number_unsigned},
                {"xtol_rel", json::value_t::number_float},
                {"ftol_rel", json::value_t::number_float},
            });
        sim_params = SimParams{}; // apply default values and overwrite them with specific ones afterward
        js.at("system_wavelength").get_to(sim_params.system_wavelength);
        if (js.contains("enable_path_loss")) { js.at("enable_path_loss").get_to(sim_params.enable_path_loss); }
        if (js.contains("n_polar")) { js.at("n_polar").get_to(sim_params.n_polar); }
        if (js.contains("n_azimuth")) { js.at("n_azimuth").get_to(sim_params.n_azimuth); }
        if (js.contains("n_linear1")) { js.at("n_linear1").get_to(sim_params.n_linear1); }
        if (js.contains("n_linear2")) { js.at("n_linear2").get_to(sim_params.n_linear2); }
        if (js.contains("xtol_rel")) { js.at("xtol_rel").get_to(sim_params.xtol_rel); }
        if (js.contains("ftol_rel")) { js.at("ftol_rel").get_to(sim_params.ftol_rel); }
    }

    void SimParams::assert_integrity() const
    {
        if (system_wavelength <= 0) throw SimulationError("Invalid numerical parameters: system_wavelength={}", system_wavelength);
        // no check for enable_path_loss
        if (n_polar <= 0) throw SimulationError("Invalid numerical parameters: n_polar={}", n_polar);
        if (n_azimuth <= 0) throw SimulationError("Invalid numerical parameters: n_azimuth={}", n_azimuth);
        if (n_linear1 <= 0) throw SimulationError("Invalid numerical parameters: n_linear1={}", n_linear1);
        if (n_linear2 <= 0) throw SimulationError("Invalid numerical parameters: n_linear2={}", n_linear2);
        if (xtol_rel <= 0) throw SimulationError("Invalid numerical parameters: xtol_rel={}", xtol_rel);
        if (ftol_rel <= 0) throw SimulationError("Invalid numerical parameters: ftol_rel={}", ftol_rel);
    }

    template void to_json(nlohmann::json&, SimParams const&);
    template void to_json(nlohmann::ordered_json&, SimParams const&);
    template void from_json(nlohmann::json const&, SimParams&);
    template void from_json(nlohmann::ordered_json const&, SimParams&);
} // namespace setup
