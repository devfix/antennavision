//
// Created by core on 18.06.26.
//

#pragma once

#include <NumCpp/NdArray/NdArrayCore.hpp>
#include <NumCpp/Rotations/Quaternion.hpp>
#include <NumCpp/Vector/Vec3.hpp>
#include <array>
#include <nlohmann/json_fwd.hpp>

namespace factory
{
    void assert_key(nlohmann::json const &js, std::string_view key, bool null_ok = false);
    bool key_exists(nlohmann::json const& js, std::string_view key);

    std::string get_string(nlohmann::json const& js, std::string_view key, bool default_ok =false);
    nc::NdArray<double> get_ndarray(nlohmann::json const &js, std::string_view key);
    char get_char(nlohmann::json const& js, std::string_view key);
    double get_double(nlohmann::json const& js, std::string_view key, bool default_ok =false);
    nc::Vec3 get_vec3(nlohmann::json const& js, std::string_view key, bool default_ok =false);
    nc::rotations::Quaternion get_quaternion(nlohmann::json const& js, std::string_view key, bool default_ok =false);
    std::array<std::string, 3> get_string_vec3(nlohmann::json const& js, std::string_view key);
} // namespace factory
