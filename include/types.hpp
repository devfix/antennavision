//
// Created by Tristan Krause on 2026-04-28.
//

#pragma once

#include <NumCpp/NdArray/NdArrayCore.hpp>
#include <NumCpp/Rotations/Quaternion.hpp>
#include <NumCpp/Vector/Vec3.hpp>
#include <complex>
#include <nlohmann/json_fwd.hpp>

using complex_t = std::complex<double>;
using NdArray = nc::NdArray<double>;
using Vec3 = nc::Vec3;
using pos_t = nc::Vec3;
using vec_t = nc::NdArray<complex_t>; /// should be of shape 3x1
using Quaternion = nc::rotations::Quaternion;
using json = nlohmann::ordered_json;
constexpr double pi = std::numbers::pi;
constexpr complex_t j = nc::constants::j;
constexpr double egamma = std::numbers::egamma;
constexpr auto sqrt2_2 = std::numbers::sqrt2 / 2.0;
static constexpr auto POS_ZERO = Vec3(0, 0, 0);
constexpr double SPEED_OF_LIGHT = 299'792'458;
constexpr double NUMERICAL_MARGIN = 1e-9;
