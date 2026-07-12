//
// Created by Tristan Krause on 2026-04-28.
//

#pragma once

#include <complex>
#include <nlohmann/json_fwd.hpp>
#include <NumCpp/Vector/Vec3.hpp>
#include <NumCpp/NdArray/NdArrayCore.hpp>
#include <NumCpp/Rotations/Quaternion.hpp>

using complex_t = std::complex<double>;
using RealArray = nc::NdArray<double>;
using ComplexArray = nc::NdArray<complex_t>;
using pos_t = nc::Vec3;
using vec_t = nc::NdArray<complex_t>; /// should be of shape 3x1
using Quaternion = nc::rotations::Quaternion;
using ojson = nlohmann::ordered_json;
using json = nlohmann::json;
constexpr double pi = std::numbers::pi;
constexpr complex_t j = nc::constants::j;
constexpr double egamma = std::numbers::egamma;
constexpr auto sqrt2_2 = std::numbers::sqrt2 / 2.0;
constexpr auto POS_ZERO = pos_t(0, 0, 0);
constexpr double SPEED_OF_LIGHT = 299'792'458;
constexpr double NUMERICAL_MARGIN = 1e-9;
