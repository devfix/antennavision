//
// Created by Tristan Krause on 2026-07-23.
//

#pragma once

#include <NumCpp/NdArray/NdArrayCore.hpp>
#include <NumCpp/Rotations/Quaternion.hpp>
#include <NumCpp/Vector/Vec3.hpp>
#include <complex>
#include <variant>

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
constexpr double NUMERICAL_MARGIN = 1e-9;
constexpr double c0 = 299'792'458; /// speed of light in vacuum
constexpr double epsilon0 = 8.854'187'818'8e-12; /// permittivity of vacuum (free space) / electric constant
constexpr double mu0 = 1.256'637'061'27e-6; /// permeability of vacuum (free space) / magnetic constant
constexpr double Z0 = mu0 * c0; /// impedance of free space
