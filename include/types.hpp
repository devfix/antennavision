//
// Created by Tristan Krause on 2026-04-28.
//

#pragma once

#include <NumCpp.hpp>
#include <complex>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "fmt/base.h"

using complex_t = std::complex<double>;
using NdArray = nc::NdArray<double>;
using Vec3 = nc::Vec3;
using Quaternion = nc::rotations::Quaternion;
using json = nlohmann::json;
using path = std::filesystem::path;
constexpr auto PI = nc::constants::pi;
static constexpr auto POS_ZERO = Vec3(0, 0, 0);
constexpr double SPEED_OF_LIGHT = 299'792'458;
constexpr double NUMERICAL_MARGIN = 1e-9;
