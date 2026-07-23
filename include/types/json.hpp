//
// Created by Tristan Krause on 2026-04-28.
//

#pragma once

#include <NumCpp/Rotations/Quaternion.hpp>
#include <NumCpp/Vector/Vec3.hpp>
#include <complex>
#include <nlohmann/json_fwd.hpp> // Lightweight forward-declarations for nlohmann::json


// json types
using ojson = nlohmann::ordered_json;
using json = nlohmann::json;
template <typename T>
concept any_json_t = std::same_as<std::decay_t<T>, json> || std::same_as<std::decay_t<T>, ojson>;


// ====================================================================================
// JSON Serializer for non-standard types
// ====================================================================================
namespace nlohmann
{
    // --- std::complex Serializer ---
    template <typename T>
    struct adl_serializer<std::complex<T>>
    {
        template <any_json_t JsonType>
        static void to_json(JsonType& js, const std::complex<T>& value);
        template <any_json_t JsonType>
        static void from_json(const JsonType& j, std::complex<T>& value);
    };

    // --- nc::Vec2 Serializer ---
    template <>
    struct adl_serializer<nc::Vec2>
    {
        template <any_json_t JsonType>
        static void to_json(JsonType& js, const nc::Vec2& value);
        template <any_json_t JsonType>
        static void from_json(const JsonType& s, nc::Vec2& value);
    };

    // --- nc::Vec3 Serializer ---
    template <>
    struct adl_serializer<nc::Vec3>
    {
        template <any_json_t JsonType>
        static void to_json(JsonType& js, const nc::Vec3& value);
        template <any_json_t JsonType>
        static void from_json(const JsonType& js, nc::Vec3& value);
    };

    // --- nc::rotations::Quaternion Serializer ---
    template <>
    struct adl_serializer<nc::rotations::Quaternion>
    {
        template <any_json_t JsonType>
        static void to_json(JsonType& js, const nc::rotations::Quaternion& value);
        template <any_json_t JsonType>
        static void from_json(const JsonType& js, nc::rotations::Quaternion& value);
    };

    // --- nc::NdArray Serializer ---
    template <typename T>
    struct adl_serializer<nc::NdArray<T>>
    {
        template <any_json_t JsonType>
        static void to_json(JsonType& js, const nc::NdArray<T>& value);
        template <any_json_t JsonType>
        static void from_json(const JsonType& j, nc::NdArray<T>& value);
    };
} // namespace nlohmann
