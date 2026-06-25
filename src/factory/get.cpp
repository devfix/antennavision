//
// Created by core on 18.06.26.
//

#include "factory/get.hpp"
#include <format>
#include <nlohmann/json.hpp>
#include "factory/parse.hpp"

namespace factory
{
    void assert_key(nlohmann::ordered_json const &js, std::string_view key, bool null_ok)
    {
        if (!js.contains(key)) { throw std::runtime_error(std::format("Error: Could not find key '{}' in json object\n{}", key, js.dump(2))); }
        if (!null_ok && js.at(key).is_null()) { throw std::runtime_error(std::format("Error: Value to key '{}' is null in json object\n{}", key, js.dump(2))); }
    }

    bool key_exists(nlohmann::ordered_json const &js, std::string_view key) { return js.contains(key); }

    void assert_empty(nlohmann::ordered_json const &js)
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

    std::string get_string(nlohmann::ordered_json &js, std::string_view key, bool remove, bool default_ok)
    {
        if (default_ok && !js.contains(key)) { return {}; }
        assert_key(js, key);
        auto val = js.at(key).get<std::string>();
        if (remove) { js.erase(key); }
        return val;
    }

    nc::NdArray<double> get_ndarray(nlohmann::ordered_json &js, std::string_view key, bool remove)
    {
        assert_key(js, key);
        auto const vect = js.at(key).get<std::vector<double>>();
        auto const array = nc::NdArray<double>(vect.begin(), vect.end());
        if (remove) { js.erase(key); }
        return array;
    }

    char get_char(nlohmann::ordered_json &js, std::string_view key, bool remove)
    {
        assert_key(js, key);
        std::string_view const str = js.at(key).get<std::string_view>();
        if (str.length() != 1) { throw std::runtime_error(std::format("Invalid character entry '{}', expected string of unity length.", str)); }
        auto const c = str.at(0);
        if (remove) { js.erase(key); }
        return c;
    }

    double get_double(nlohmann::ordered_json &js, std::string_view key, std::map<std::string, double> const& variables, bool remove, bool default_ok)
    {
        if (default_ok && !js.contains(key)) { return 0.0; }
        assert_key(js, key);
        if (js.at(key).is_string())
        {
                auto const val = parse_double(js.at(key).get<std::string>(), variables);
                if (remove) { js.erase(key); }
                return val;
        }
        if (js.at(key).is_number())
        {
            auto const val = js.at(key).get<double>();
            if (remove) { js.erase(key); }
            return val;
        }
        throw std::runtime_error(std::format("Invalid type '{}' of entry '{}'", js.at(key).type_name(), key));
    }

    std::uint32_t get_uint(nlohmann::ordered_json &js, std::string_view key, std::map<std::string, double> const& variables, bool remove, bool default_ok)
    {
        if (default_ok && !js.contains(key)) { return 0.0; }
        assert_key(js, key);
        if (js.at(key).is_string())
        {
                auto const val = static_cast<std::uint32_t>(std::round(variables.at(js.at(key).get<std::string>())));
                if (remove) { js.erase(key); }
                return val;
        }
        if (js.at(key).is_number_integer())
        {
            auto const val = js.at(key).get<std::uint32_t>();
            if (remove) { js.erase(key); }
            return val;
        }
        throw std::runtime_error(std::format("Invalid type '{}' of entry '{}'", js.at(key).type_name(), key));
    }

    nc::Vec3 get_vec3(nlohmann::ordered_json &js, std::string_view key, std::map<std::string, double> const& variables, bool remove, bool default_ok)
    {
        if (default_ok && !js.contains(key)) { return {}; }
        assert_key(js, key);
        if (js.at(key).is_array()) { return get_ndarray(js, key); }
        auto const val = nc::Vec3(get_double(js.at(key), "x", variables), get_double(js.at(key), "y", variables), get_double(js.at(key), "z", variables));
        if (remove) { js.erase(key); }
        return val;
    }

    nc::rotations::Quaternion get_quaternion(nlohmann::ordered_json &js, std::string_view key, std::map<std::string, double> const& variables, bool remove, bool default_ok)
    {
        if (default_ok && !js.contains(key)) { return {}; }
        assert_key(js, key);
        if (js.at(key).is_array()) { return (nc::Vec3(get_ndarray(js, key)) * nc::constants::pi).toNdArray(); } // intermediate step via Vec3 to ensure correct array shape
        auto const val = nc::rotations::Quaternion(get_double(js.at(key), "roll", variables) * nc::constants::pi, get_double(js.at(key), "pitch", variables) * nc::constants::pi,
                                                   get_double(js.at(key), "yaw", variables) * nc::constants::pi);
        if (remove) { js.erase(key); }
        return val;
    }

    std::array<std::string, 3> get_string_vec3(nlohmann::ordered_json &js, std::string_view key, bool remove)
    {
        assert_key(js, key);
        auto const val = js.at(key).get<std::array<std::string, 3>>();
        if (remove) { js.erase(key); }
        return val;
    }

} // namespace factory
