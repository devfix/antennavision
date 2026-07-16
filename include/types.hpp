//
// Created by Tristan Krause on 2026-04-28.
//

#pragma once

#include <complex>
#include <nlohmann/json_fwd.hpp> // Lightweight forward-declarations for nlohmann::json
#include <NumCpp/Vector/Vec3.hpp>
#include <NumCpp/NdArray/NdArrayCore.hpp>
#include <NumCpp/Rotations/Quaternion.hpp>

// single-element types
using complex_t = std::complex<double>;
using pos_t = nc::Vec3;
using vec_t = nc::NdArray<complex_t>; /// should be of shape 3x1
using Quaternion = nc::rotations::Quaternion;

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

// json serialization

namespace nlohmann {
    // --- std::complex Serializer ---
    template <typename T>
    struct adl_serializer<std::complex<T>> {
        template<any_json_t JsonType>
        static void to_json(JsonType& j, const std::complex<T>& value);
        template<any_json_t JsonType>
        static void from_json(const JsonType& j, std::complex<T>& value);
    };

    // --- nc::Vec2 Serializer ---
    template <>
    struct adl_serializer<nc::Vec2> {
        template<any_json_t JsonType>
        static void to_json(JsonType& j, const nc::Vec2& value);
        template<any_json_t JsonType>
        static void from_json(const JsonType& j, nc::Vec2& value);
    };

    // --- nc::Vec3 Serializer ---
    template <>
    struct adl_serializer<nc::Vec3> {
        template<any_json_t JsonType>
        static void to_json(JsonType& j, const nc::Vec3& value);
        template<any_json_t JsonType>
        static void from_json(const JsonType& j, nc::Vec3& value);
    };
} // namespace nlohmann

namespace nc {
    template <any_json_t JsonType, typename T>
    void to_json(JsonType& j, NdArray<T> const& array);

    template <any_json_t JsonType, typename T>
    void from_json(JsonType const& j, NdArray<T>& array);
} // namespace nc
