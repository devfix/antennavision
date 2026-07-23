//
// Created by core on 18.06.26.
//

#pragma once

#include <functional>
#include <map>
#include <nlohmann/json.hpp>
#include <string>

#include "types/math.hpp"
#include "types/json.hpp"
#include "simulationerror.hpp"

namespace factory
{
    template <typename>
    struct dependent_false_ : std::false_type
    {};

    std::function<complex_t(double polar, double azimuth, double wavelength)> parse_polar_azimuth_function(std::string const& expr);
    double parse_double(std::string const& expr, std::map<std::string, var_t> const& variables);
    std::int64_t parse_int(std::string const& expr, std::map<std::string, var_t> const& variables);

    template <typename TargetType, any_json_t AnyJson>
    void try_resolve_expressions(AnyJson& js, std::map<std::string, var_t> const& variables, std::string_view key = "")
    {
        if (js.is_object() && !key.empty() && js.contains(key))
        {
            try
            {
                try_resolve_expressions<TargetType, AnyJson>(js[key], variables);
            }catch (...)
            {
                std::throw_with_nested(SimulationError("Failed to parse JSON field '{}'", key));
            }
        }
        else if (js.is_object() && key.empty())
        {
            for (auto& [sub_key, value] : js.items()) { try_resolve_expressions<TargetType, AnyJson>(value, variables); }
        }
        else if (js.is_array())
        {
            for (auto& e : js) { try_resolve_expressions<TargetType, AnyJson>(e, variables); }
        }
        else if (js.is_string())
        {
            if constexpr (std::is_same_v<TargetType, double>) { js = parse_double(js.template get<std::string>(), variables); }
            else if constexpr (std::is_same_v<TargetType, std::int64_t>) { js = parse_int(js.template get<std::string>(), variables); }
            else
            {
                static_assert(dependent_false_<TargetType>::value, "Invalid target type");
            }
        }
    }

    template <any_json_t AnyJson>
    void try_resolve_double_expressions(AnyJson& js, std::map<std::string, var_t> const& variables, std::string_view key = "")
    {
        try_resolve_expressions<double>(js, variables, key);
    }

    template <any_json_t AnyJson>
    void try_resolve_int_expressions(AnyJson& js, std::map<std::string, var_t> const& variables, std::string_view key = "")
    {
        try_resolve_expressions<std::int64_t>(js, variables, key);
    }
} // namespace factory
