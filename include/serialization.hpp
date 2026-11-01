//
// Created by Tristan Krause on 2026-07-16.
//

#pragma once

#include <complex>
#include <string_view>
#include "types/json.hpp"
#include "simulationerror.hpp"
#include "reference.hpp"

namespace serialization
{
    enum struct FileType
    {
        JSON,
        BSON,
        CBOR,
        MSGPACK,
        UBJSON
    };

    [[nodiscard]] std::optional<FileType> file_type_from_ext(std::string_view ext);
    [[nodiscard]] std::string_view ext_from_file_type(FileType output_type);

    template <AnyJson JsonType>
    JsonType load_file(std::filesystem::path const& path);

    template <AnyJson JsonType>
    void store_file(JsonType const& js, std::filesystem::path const& path);

    struct JsonField
    {
        template <typename T>
        constexpr JsonField(char const* name, T type) : name(name), types({static_cast<std::uint8_t>(type)})
        { add_additional_types();}

        template <typename T>
        constexpr JsonField(char const* name, std::initializer_list<T> types_lst) : name(name)
        {
            std::ranges::transform(types_lst, std::back_insert_iterator(types), [](T t) { return static_cast<std::uint8_t>(t); });
            add_additional_types();
        }

        void add_additional_types();

        char const* name;
        std::vector<std::uint8_t> types;
    };

    template <AnyJson JsonType>
    void assert_structure(JsonType const& js, std::string_view structure_name, std::vector<JsonField> const& mandatory, std::vector<JsonField> const& optional);
} // namespace serialization


// ====================================================================================
// JSON Serializer for non-standard 3rd party types
// ====================================================================================
namespace nlohmann
{
    // --- std::complex Serializer ---
    template <typename T>
    struct adl_serializer<std::complex<T>>
    {
        template <AnyJson JsonType>
        static void to_json(JsonType& js, const std::complex<T>& value);
        template <AnyJson JsonType>
        static void from_json(const JsonType& j, std::complex<T>& value);
    };

    // --- nc::Vec2 Serializer ---
    template <>
    struct adl_serializer<nc::Vec2>
    {
        template <AnyJson JsonType>
        static void to_json(JsonType& js, const nc::Vec2& value);
        template <AnyJson JsonType>
        static void from_json(const JsonType& s, nc::Vec2& value);
    };

    // --- nc::Vec3 Serializer ---
    template <>
    struct adl_serializer<nc::Vec3>
    {
        template <AnyJson JsonType>
        static void to_json(JsonType& js, const nc::Vec3& value);
        template <AnyJson JsonType>
        static void from_json(const JsonType& js, nc::Vec3& value);
    };

    // --- nc::rotations::Quaternion Serializer ---
    template <>
    struct adl_serializer<nc::rotations::Quaternion>
    {
        template <AnyJson JsonType>
        static void to_json(JsonType& js, const nc::rotations::Quaternion& value);
        template <AnyJson JsonType>
        static void from_json(const JsonType& js, nc::rotations::Quaternion& value);
    };

    // --- nc::NdArray Serializer ---
    template <typename T>
    struct adl_serializer<nc::NdArray<T>>
    {
        template <AnyJson JsonType>
        static void to_json(JsonType& js, const nc::NdArray<T>& value);
        template <AnyJson JsonType>
        static void from_json(const JsonType& j, nc::NdArray<T>& value);
    };
} // namespace nlohmann
