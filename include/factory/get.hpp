//
// Created by core on 18.06.26.
//

#pragma once

#include <array>
#include <cstdint>
#include <NumCpp/NdArray/NdArrayCore.hpp>
#include <NumCpp/Rotations/Quaternion.hpp>
#include <NumCpp/Vector/Vec3.hpp>
#include <nlohmann/json_fwd.hpp>

namespace factory
{
    void assert_key(nlohmann::ordered_json const&js, std::string_view key, bool null_ok = false);
    bool key_exists(nlohmann::ordered_json const& js, std::string_view key);
    void assert_empty(nlohmann::ordered_json const& js);

    std::string get_string(nlohmann::ordered_json& js, std::string_view key, bool remove=true, bool default_ok =false);
    nc::NdArray<double> get_ndarray(nlohmann::ordered_json &js, std::string_view key, bool remove=true);
    char get_char(nlohmann::ordered_json & js, std::string_view key, bool remove=true);
    double get_double(nlohmann::ordered_json & js, std::string_view key, std::map<std::string, double> const& variables, bool remove=true, bool default_ok =false);
    std::uint32_t get_uint(nlohmann::ordered_json & js, std::string_view key, std::map<std::string, double> const& variables, bool remove=true, bool default_ok =false);
    nc::Vec3 get_vec3(nlohmann::ordered_json & js, std::string_view key, std::map<std::string, double> const& variables, bool remove=true, bool default_ok =false);
    nc::rotations::Quaternion get_quaternion(nlohmann::ordered_json & js, std::string_view key, std::map<std::string, double> const& variables, bool remove=true, bool default_ok =false);
    std::array<std::string, 3> get_string_vec3(nlohmann::ordered_json & js, std::string_view key, bool remove=true);
} // namespace factory
