//
// Created by core on 2026-07-23.
//

#pragma once

#include <complex>
#include <variant>
#include <NumCpp/NdArray/NdArrayCore.hpp>
#include <NumCpp/Rotations/Quaternion.hpp>
#include <NumCpp/Vector/Vec3.hpp>


// single-element types
using complex_t = std::complex<double>;
using pos_t = nc::Vec3;
using vec_t = nc::NdArray<complex_t>; /// should be of shape 3x1
using Quaternion = nc::rotations::Quaternion;
using var_t = std::variant<double, std::int64_t>;

// array types
using RealArray = nc::NdArray<double>;
using ComplexArray = nc::NdArray<complex_t>;
using PositionArray = nc::NdArray<pos_t>;
using SurfacePositionArray = nc::NdArray<nc::Vec2>;
using VectorArray = nc::NdArray<vec_t>;
using QuaternionArray = nc::NdArray<Quaternion>; // probably never used but we already define it here

// mathematical and physical constants
constexpr double pi = std::numbers::pi;
constexpr complex_t j = nc::constants::j;
constexpr double egamma = std::numbers::egamma;
constexpr auto sqrt2_2 = std::numbers::sqrt2 / 2.0;
constexpr auto POS_ZERO = pos_t(0, 0, 0);
constexpr double SPEED_OF_LIGHT = 299'792'458;
constexpr double NUMERICAL_MARGIN = 1e-9;
