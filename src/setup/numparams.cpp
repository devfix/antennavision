//
// Created by Tristan Krause on 2026-07-24.
//

#include "setup/numparams.hpp"
#include <nlohmann/json.hpp>
#include "serialization.hpp"


namespace setup
{
    template <AnyJson JsonType>
    void to_json(JsonType& js, NumParams const& num_params)
    {
        js = JsonType{{"system_wavelength", num_params.system_wavelength},
            {"n_polar", num_params.n_polar},
            {"n_azimuth", num_params.n_azimuth},
            {"n_linear1", num_params.n_linear1},
            {"n_linear2", num_params.n_linear2},
            {"xtol_rel", num_params.xtol_rel},
            {"ftol_rel", num_params.ftol_rel}};
    }

    template <AnyJson JsonType>
    void from_json(JsonType const& js, NumParams& num_params)
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
        num_params = DEFAULT_NUM_PARAMS; // apply default values and overwrite them with specific ones afterward
        js.at("system_wavelength").get_to(num_params.system_wavelength);
        if (js.contains("n_polar")) { js.at("n_polar").get_to(num_params.n_polar); }
        if (js.contains("n_azimuth")) { js.at("n_azimuth").get_to(num_params.n_azimuth); }
        if (js.contains("n_linear1")) { js.at("n_linear1").get_to(num_params.n_linear1); }
        if (js.contains("n_linear2")) { js.at("n_linear2").get_to(num_params.n_linear2); }
        if (js.contains("xtol_rel")) { js.at("xtol_rel").get_to(num_params.xtol_rel); }
        if (js.contains("ftol_rel")) { js.at("ftol_rel").get_to(num_params.ftol_rel); }
    }

    NumParams NumParams::configure(NumParams const& num_params)
    {
        NumParams copy = DEFAULT_NUM_PARAMS;
        if (num_params.system_wavelength != 0) { copy.system_wavelength = num_params.system_wavelength; }
        if (num_params.n_polar != 0) { copy.n_polar = num_params.n_polar; }
        if (num_params.n_azimuth != 0) { copy.n_azimuth = num_params.n_azimuth; }
        if (num_params.n_linear1 != 0) { copy.n_linear1 = num_params.n_linear1; }
        if (num_params.n_linear2 != 0) { copy.n_linear2 = num_params.n_linear2; }
        if (num_params.xtol_rel != 0) { copy.xtol_rel = num_params.xtol_rel; }
        if (num_params.ftol_rel != 0) { copy.system_wavelength = num_params.ftol_rel; }
        copy.check();
        return copy;
    }

    void NumParams::check() const
    {
        assert(system_wavelength > 0);
        assert(n_polar > 0);
        assert(n_azimuth > 0);
        assert(n_linear1 > 0);
        assert(n_linear2 > 0);
        assert(xtol_rel > 0);
        assert(ftol_rel > 0);
    }
    
    template void to_json(nlohmann::json&, NumParams const&);
    template void to_json(nlohmann::ordered_json&, NumParams const&);
    template void from_json(nlohmann::json const&, NumParams&);
    template void from_json(nlohmann::ordered_json const&, NumParams&);
} // namespace setup
