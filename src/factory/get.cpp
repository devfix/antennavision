//
// Created by core on 18.06.26.
//

#include "factory/get.hpp"
#include <format>
#include <nlohmann/json.hpp>

namespace factory
{
    void assert_key(nlohmann::json const &js, std::string_view key, bool null_ok)
    {
        if (!js.contains(key)) { throw std::runtime_error(std::format("Error: Could not find key '{}' in json object\n{}", key, js.dump(2))); }
        if (!null_ok && js[key].is_null()) { throw std::runtime_error(std::format("Error: Value to key '{}' is null in json object\n{}", key, js.dump(2))); }
    }

    bool key_exists(nlohmann::json const &js, std::string_view key) { return js.contains(key); }

    std::string get_string(nlohmann::json const &js, std::string_view key, bool default_ok)
    {
        if (default_ok && !js.contains(key)) { return {}; }
        assert_key(js, key);
        return js[key].get<std::string>();
    }

    nc::NdArray<double> get_ndarray(nlohmann::json const &js, std::string_view key)
    {
        assert_key(js, key);
        auto const phis = js[key].get<std::vector<double>>();
        return nc::NdArray<double>(phis.begin(), phis.end());
    }

    char get_char(nlohmann::json const &js, std::string_view key)
    {
        assert_key(js, key);
        std::string_view const str = js[key].get<std::string_view>();
        if (str.length() != 1) { throw std::runtime_error(std::format("Invalid character entry '{}', expected string of unity length.", str)); }
        return str.at(0);
    }

    double get_double(nlohmann::json const &js, std::string_view key, bool default_ok)
    {
        if (default_ok && !js.contains(key)) { return 0.0; }
        assert_key(js, key);
        return js[key].get<double>();
    }

    nc::Vec3 get_vec3(nlohmann::json const &js, std::string_view key, bool default_ok)
    {
        if (default_ok && !js.contains(key)) { return {}; }
        assert_key(js, key);
        if (js[key].is_array()) { return get_ndarray(js, key); }
        return {get_double(js[key], "x"), get_double(js[key], "y"), get_double(js[key], "z")};
    }

    nc::rotations::Quaternion get_quaternion(nlohmann::json const &js, std::string_view key, bool default_ok)
    {
        if (default_ok && !js.contains(key)) { return {}; }
        assert_key(js, key);
        if (js[key].is_array()) { return (nc::Vec3(get_ndarray(js, key)) * nc::constants::pi).toNdArray(); } // intermediate step via Vec3 to ensure correct array shape
        return {get_double(js[key], "roll") * nc::constants::pi, get_double(js[key], "pitch") * nc::constants::pi, get_double(js[key], "yaw") * nc::constants::pi};
    }

    std::array<std::string, 3> get_string_vec3(nlohmann::json const &js, std::string_view key)
    {
        assert_key(js, key);
        return js[key].get<std::array<std::string, 3>>();
    }

} // namespace factory
