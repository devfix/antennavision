//
// Created by core on 18.06.26.
//

#include "factory/get.hpp"
#include <format>
#include <nlohmann/json.hpp>
#include <ranges>

namespace factory
{
    void assert_key(nlohmann::json const &js, std::string_view key, bool null_ok)
    {
        if (!js.contains(key)) { throw std::runtime_error(std::format("Error: Could not find key '{}' in json object\n{}", key, js.dump(2))); }
        if (!null_ok && js[key].is_null()) { throw std::runtime_error(std::format("Error: Value to key '{}' is null in json object\n{}", key, js.dump(2))); }
    }

    bool key_exists(nlohmann::json const &js, std::string_view key) { return js.contains(key); }

    void assert_empty(nlohmann::json const &js)
    {
        if (js.empty()) { return; }
        std::string bad_keys("Invalid keys found: ");
        for (auto it = js.begin(); it != js.end();)
        {
            bad_keys.append(std::format("'{}'", it.key()));
            if (++it != js.end()) { bad_keys.append(", "); }
        }
        throw std::runtime_error(bad_keys);
    }

    std::string get_string(nlohmann::json &js, std::string_view key, bool remove, bool default_ok)
    {
        if (default_ok && !js.contains(key)) { return {}; }
        assert_key(js, key);
        auto val = js[key].get<std::string>();
        if (remove) { js.erase(key); }
        return val;
    }

    nc::NdArray<double> get_ndarray(nlohmann::json &js, std::string_view key, bool remove)
    {
        assert_key(js, key);
        auto const phis = js[key].get<std::vector<double>>();
        auto const val = nc::NdArray<double>(phis.begin(), phis.end());
        if (remove) { js.erase(key); }
        return val;
    }

    char get_char(nlohmann::json &js, std::string_view key, bool remove)
    {
        assert_key(js, key);
        std::string_view const str = js[key].get<std::string_view>();
        if (str.length() != 1) { throw std::runtime_error(std::format("Invalid character entry '{}', expected string of unity length.", str)); }
        auto const c = str.at(0);
        if (remove) { js.erase(key); }
        return c;
    }

    double get_double(nlohmann::json &js, std::string_view key, bool remove, bool default_ok)
    {
        if (default_ok && !js.contains(key)) { return 0.0; }
        assert_key(js, key);
        auto const val = js[key].get<double>();
        if (remove) { js.erase(key); }
        return val;
    }

    nc::Vec3 get_vec3(nlohmann::json &js, std::string_view key, bool remove, bool default_ok)
    {
        if (default_ok && !js.contains(key)) { return {}; }
        assert_key(js, key);
        if (js[key].is_array()) { return get_ndarray(js, key); }
        auto const val = nc::Vec3(get_double(js[key], "x"), get_double(js[key], "y"), get_double(js[key], "z"));
        if (remove) { js.erase(key); }
        return val;
    }

    nc::rotations::Quaternion get_quaternion(nlohmann::json &js, std::string_view key, bool remove, bool default_ok)
    {
        if (default_ok && !js.contains(key)) { return {}; }
        assert_key(js, key);
        if (js[key].is_array()) { return (nc::Vec3(get_ndarray(js, key)) * nc::constants::pi).toNdArray(); } // intermediate step via Vec3 to ensure correct array shape
        auto const val = nc::rotations::Quaternion(get_double(js[key], "roll") * nc::constants::pi, get_double(js[key], "pitch") * nc::constants::pi, get_double(js[key], "yaw") * nc::constants::pi);
        if (remove) { js.erase(key); }
        return val;
    }

    std::array<std::string, 3> get_string_vec3(nlohmann::json &js, std::string_view key, bool remove)
    {
        assert_key(js, key);
        auto const val = js[key].get<std::array<std::string, 3>>();
        if (remove) { js.erase(key); }
        return val;
    }

} // namespace factory
