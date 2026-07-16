//
// Created by Tristan Krause on 2026-04-28.
//

#pragma once

#include <stdfloat>
#include <cstdint>
#include <complex>
#include <variant>
#include <nlohmann/json_fwd.hpp> // Lightweight forward-declarations for nlohmann::json
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

// other types
using ojson = nlohmann::ordered_json;
using json = nlohmann::json;
template <typename T>
concept any_json_t = std::same_as<std::decay_t<T>, json> || std::same_as<std::decay_t<T>, ojson>;

// mathematical and physical constants
constexpr double pi = std::numbers::pi;
constexpr complex_t j = nc::constants::j;
constexpr double egamma = std::numbers::egamma;
constexpr auto sqrt2_2 = std::numbers::sqrt2 / 2.0;
constexpr auto POS_ZERO = pos_t(0, 0, 0);
constexpr double SPEED_OF_LIGHT = 299'792'458;
constexpr double NUMERICAL_MARGIN = 1e-9;


// ====================================================================================
// JSON Serializer for non-standard types
// ====================================================================================
namespace nlohmann {
    // --- std::complex Serializer ---
    template <typename T>
    struct adl_serializer<std::complex<T>> {
        template<any_json_t JsonType>
        static void to_json(JsonType& js, const std::complex<T>& value);
        template<any_json_t JsonType>
        static void from_json(const JsonType& j, std::complex<T>& value);
    };

    // --- nc::Vec2 Serializer ---
    template <>
    struct adl_serializer<nc::Vec2> {
        template<any_json_t JsonType>
        static void to_json(JsonType& js, const nc::Vec2& value);
        template<any_json_t JsonType>
        static void from_json(const JsonType& s, nc::Vec2& value);
    };

    // --- nc::Vec3 Serializer ---
    template <>
    struct adl_serializer<nc::Vec3> {
        template<any_json_t JsonType>
        static void to_json(JsonType& js, const nc::Vec3& value);
        template<any_json_t JsonType>
        static void from_json(const JsonType& js, nc::Vec3& value);
    };

    // --- nc::rotations::Quaternion Serializer ---
    template <>
    struct adl_serializer<nc::rotations::Quaternion> {
        template<any_json_t JsonType>
        static void to_json(JsonType& js, const nc::rotations::Quaternion& value);
        template<any_json_t JsonType>
        static void from_json(const JsonType& js, nc::rotations::Quaternion& value);
    };

    // --- nc::NdArray Serializer ---
    template <typename T>
    struct adl_serializer<nc::NdArray<T>> {
        template<any_json_t JsonType>
        static void to_json(JsonType& js, const nc::NdArray<T>& value);
        template<any_json_t JsonType>
        static void from_json(const JsonType& j, nc::NdArray<T>& value);
    };
} // namespace nlohmann
