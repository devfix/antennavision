//
// Created by Tristan Krause on 2026-07-23.
//

#pragma once

#include <complex>
#include <variant>
#include <NumCpp/NdArray/NdArrayCore.hpp>
#include <NumCpp/Rotations/Quaternion.hpp>
#include <NumCpp/Vector/Vec3.hpp>

// single-element types
using Complex = std::complex<double>;
using Pos = nc::Vec3;
using Vec = nc::NdArray<Complex>; /// should be of shape 3x1
using Quaternion = nc::rotations::Quaternion;

// array types
using RealArray = nc::NdArray<double>;
using ComplexArray = nc::NdArray<Complex>;
using Vec2Array = nc::NdArray<nc::Vec2>;
using Vec3Array = nc::NdArray<nc::Vec3>;

// mathematical and physical constants
constexpr double pi = std::numbers::pi;
constexpr Complex j = nc::constants::j;
constexpr double egamma = std::numbers::egamma;
constexpr auto sqrt2_2 = std::numbers::sqrt2 / 2.0;
constexpr auto POS_ZERO = Pos(0, 0, 0);
constexpr double SPEED_OF_LIGHT = 299'792'458;
constexpr double NUMERICAL_MARGIN = 1e-9;
