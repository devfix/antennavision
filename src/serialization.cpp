//
// Created by Tristan Krause on 2026-07-16.
//

#include "serialization.hpp"
#include <algorithm>
#include <nlohmann/json.hpp>

struct Reference;

namespace serialization
{
    namespace
    {
        template <typename T>
        void assert_field(const T& j, std::string_view structure_name, const char* name, std::vector<std::uint8_t> expected_types_int, bool mandatory)
        {
            using std::ranges::to;
            using std::views::join_with;
            using std::views::transform;

            auto expected_types = expected_types_int | transform([](auto const& v) { return static_cast<json::value_t>(v); }) | to<std::vector>();

            if (!j.contains(name))
            {
                if (mandatory) { throw SimulationError("Failed to load {} from JSON: missing mandatory field '{}'", structure_name, name); }
                return;
            }

            auto type = j.at(name).type();
            if (!std::ranges::contains(expected_types, type))
            {
                std::string const expected_types_str = expected_types //
                    | transform([](auto const& t) { return std::string(nlohmann::json(t).type_name()); }) //
                    | join_with(std::string_view{" or "}) //
                    | to<std::string>();
                throw SimulationError("Failed to load {} from JSON, expected type {} for field '{}' but got {}",
                    structure_name,
                    expected_types_str,
                    name,
                    j.at(name).type_name());
            }
        }
    } // namespace

    template <AnyJson JsonType>
    void assert_structure(JsonType const& js, std::string_view structure_name, std::vector<JsonField> const& mandatory, std::vector<JsonField> const& optional)
    {
        if (!js.is_object()) { throw SimulationError("Failed to load {} from JSON, expected JSON object, got {}", structure_name, js.type_name()); }
        for (auto const& field : mandatory) { assert_field(js, structure_name, field.name, field.types, true); }
        for (auto const& field : optional) { assert_field(js, structure_name, field.name, field.types, false); }
        for (auto const& [key, value] : js.items())
        {
            if (!std::ranges::contains(mandatory, key, &JsonField::name) and !std::ranges::contains(optional, key, &JsonField::name))
            {
                throw SimulationError("Failed to load {} from JSON, invalid JSON entry '{}' of type '{}' found", structure_name, key, value.type_name());
            }
        }
    }

    void JsonField::add_additional_types()
    {
        using std::ranges::contains;
        auto constexpr type_float = static_cast<std::uint8_t>(json::value_t::number_float);
        auto constexpr type_integer = static_cast<std::uint8_t>(json::value_t::number_integer);
        auto constexpr type_unsigned = static_cast<std::uint8_t>(json::value_t::number_unsigned);
        if (contains(types, type_float) and !contains(types, type_integer)) types.push_back(type_integer);
        if (contains(types, type_float) and !contains(types, type_unsigned)) types.push_back(type_unsigned);
        if (contains(types, type_integer) and !contains(types, type_unsigned)) types.push_back(type_unsigned);
    }

    template void
    assert_structure(json const& j, std::string_view structure_name, std::vector<JsonField> const& mandatory, std::vector<JsonField> const& optional);
    template void
    assert_structure(ojson const& j, std::string_view structure_name, std::vector<JsonField> const& mandatory, std::vector<JsonField> const& optional);
} // namespace serialization


// ====================================================================================
// JSON Serializer for non-standard 3rd party types
// ====================================================================================
namespace nlohmann {
    // ------------------------------------------------------------------------------------
    // std::complex Serializer
    // ------------------------------------------------------------------------------------
    template<typename T>
    template<AnyJson JsonType>
    void adl_serializer<std::complex<T>>::to_json(JsonType& js, const std::complex<T>& value) {
        js = JsonType{ value.real(), value.imag() };
    }
    template<typename T>
    template<AnyJson JsonType>
    void adl_serializer<std::complex<T>>::from_json(const JsonType& j, std::complex<T>& value) {
        if (j.is_array() && j.size() == 2) {
            value.real(j.at(0).template get<T>());
            value.imag(j.at(1).template get<T>());
        } else {
            throw JsonType::type_error::create(302, "Expected a 2-element array for std::complex", &j);
        }
    }

    // Instantiations for std::complex<double>
    template void adl_serializer<Complex>::to_json(json&, Complex const&);
    template void adl_serializer<Complex>::to_json(ordered_json&, Complex const&);
    template void adl_serializer<Complex>::from_json(json const&, Complex&);
    template void adl_serializer<Complex>::from_json(ordered_json const&, Complex&);

    // ------------------------------------------------------------------------------------
    // nc::Vec2 Serializer
    // ------------------------------------------------------------------------------------
    template<AnyJson JsonType>
    void adl_serializer<nc::Vec2>::to_json(JsonType& js, const nc::Vec2& value) {
        js = JsonType{ value.x, value.y };
    }
    template<AnyJson JsonType>
    void adl_serializer<nc::Vec2>::from_json(const JsonType& s, nc::Vec2& value) {
        if (s.is_array() && s.size() == 2) {
            s[0].get_to(value.x);
            s[1].get_to(value.y);
        } else {
            throw JsonType::type_error::create(302, "Expected a 2-element array for nc::Vec2", &s);
        }
    }

    // Instantiations for nc::Vec2
    template void adl_serializer<nc::Vec2>::to_json(json&, nc::Vec2 const&);
    template void adl_serializer<nc::Vec2>::to_json(ordered_json&, nc::Vec2 const&);
    template void adl_serializer<nc::Vec2>::from_json(json const&, nc::Vec2&);
    template void adl_serializer<nc::Vec2>::from_json(ordered_json const&, nc::Vec2&);

    // ------------------------------------------------------------------------------------
    // nc::Vec3 Serializer
    // ------------------------------------------------------------------------------------
    template<AnyJson JsonType>
    void adl_serializer<nc::Vec3>::to_json(JsonType& js, const nc::Vec3& value) {
        js = JsonType{ value.x, value.y, value.z };
    }
    template<AnyJson JsonType>
    void adl_serializer<nc::Vec3>::from_json(const JsonType& js, nc::Vec3& value) {
        if (js.is_array() && js.size() == 3) {
            js[0].get_to(value.x);
            js[1].get_to(value.y);
            js[2].get_to(value.z);
        } else {
            throw JsonType::type_error::create(302, "Expected a 3-element array for nc::Vec3", &js);
        }
    }

    // Instantiations for nc::Vec3
    template void adl_serializer<nc::Vec3>::to_json(json&, nc::Vec3 const&);
    template void adl_serializer<nc::Vec3>::to_json(ordered_json&, nc::Vec3 const&);
    template void adl_serializer<nc::Vec3>::from_json(json const&, nc::Vec3&);
    template void adl_serializer<nc::Vec3>::from_json(ordered_json const&, nc::Vec3&);

    // ------------------------------------------------------------------------------------
    // nc::rotations::Quaternion Serializer
    // ------------------------------------------------------------------------------------
    template<AnyJson JsonType>
    void adl_serializer<Quaternion>::to_json(JsonType& js, const Quaternion& value) {
        // Always serialize as a 4-element array [s, i, j, k]
        js = JsonType{ value.s(), value.i(), value.j(), value.k() };
    }

    template<AnyJson JsonType>
    void adl_serializer<Quaternion>::from_json(const JsonType& js, Quaternion& value)
    {
        if (js.is_array() && js.size() == 4) {
            // Case 1: Read raw 4-element quaternion representation
            double const cs = js.at(0).template get<double>();
            double const ci = js.at(1).template get<double>();
            double const cj = js.at(2).template get<double>();
            double const ck = js.at(3).template get<double>();
            value = Quaternion(ci, cj, ck, cs);
        }
        else if (js.is_object() && js.contains("roll") && js.contains("pitch") && js.contains("yaw")) {
            // Case 2: Read fallback Roll-Pitch-Yaw object representation
            double const roll = js.at("roll").template get<double>();
            double const pitch = js.at("pitch").template get<double>();
            double const yaw = js.at("yaw").template get<double>();
            value = Quaternion(roll, pitch, yaw);
        }
        else {
            throw JsonType::type_error::create(
                302,
                "Expected a 4-element array [s, x, y, z] or a roll-pitch-yaw object for Quaternion",
                &js
            );
        }
    }

    // Instantiations for Quaternion
    template void adl_serializer<Quaternion>::to_json(json&, Quaternion const&);
    template void adl_serializer<Quaternion>::to_json(ordered_json&, Quaternion const&);
    template void adl_serializer<Quaternion>::from_json(json const&, Quaternion&);
    template void adl_serializer<Quaternion>::from_json(ordered_json const&, Quaternion&);

    // ------------------------------------------------------------------------------------
    // nc::NdArray<T> Serializer
    // ------------------------------------------------------------------------------------
    template<typename T>
    template<AnyJson JsonType>
    void adl_serializer<nc::NdArray<T>>::to_json(JsonType& js, const nc::NdArray<T>& value) {
        auto shape = value.shape();
        js = JsonType::array();

        for (std::uint32_t row = 0; row < shape.rows; ++row) {
            JsonType inner_row = JsonType::array();
            for (std::uint32_t col = 0; col < shape.cols; ++col) {
                // This will automatically find the correct ADL serializer
                // for the element type T (e.g., complex_t or double)
                inner_row.push_back(value(row, col));
            }
            js.push_back(inner_row);
        }
    }

    template<typename T>
    template<AnyJson JsonType>
    void adl_serializer<nc::NdArray<T>>::from_json(const JsonType& j, nc::NdArray<T>& value) {
        if (!j.is_array()) {
            throw JsonType::type_error::create(302, "Expected a 2D JSON array for nc::NdArray", &j);
        }

        std::uint32_t num_rows = static_cast<std::uint32_t>(j.size());
        if (num_rows == 0) {
            value = nc::NdArray<T>();
            return;
        }

        // Determine column count from the first row
        if (!j.at(0).is_array()) {
            throw JsonType::type_error::create(302, "Expected a nested array structure for nc::NdArray", &j);
        }
        std::uint32_t num_cols = static_cast<std::uint32_t>(j.at(0).size());

        // Create an NdArray of correct shape
        value = nc::NdArray<T>(num_rows, num_cols);

        for (std::uint32_t r = 0; r < num_rows; ++r) {
            const auto& row_js = j.at(r);
            if (!row_js.is_array() || row_js.size() != num_cols) {
                throw JsonType::type_error::create(302, "Inconsistent column dimensions in nc::NdArray JSON", &j);
            }
            for (std::uint32_t c = 0; c < num_cols; ++c) {
                value(r, c) = row_js.at(c).template get<T>(); // Recursively gets element T
            }
        }
    }

    // Explicit Instantiations for nc::NdArray<double>
    template struct adl_serializer<nc::NdArray<double>>;
    template void adl_serializer<nc::NdArray<double>>::to_json(json&, const nc::NdArray<double>&);
    template void adl_serializer<nc::NdArray<double>>::to_json(ordered_json&, const nc::NdArray<double>&);
    template void adl_serializer<nc::NdArray<double>>::from_json(const json&, nc::NdArray<double>&);
    template void adl_serializer<nc::NdArray<double>>::from_json(const ordered_json&, nc::NdArray<double>&);

    // Explicit Instantiations for nc::NdArray<complex_t>
    template struct adl_serializer<nc::NdArray<Complex>>;
    template void adl_serializer<nc::NdArray<Complex>>::to_json(json&, const nc::NdArray<Complex>&);
    template void adl_serializer<nc::NdArray<Complex>>::to_json(ordered_json&, const nc::NdArray<Complex>&);
    template void adl_serializer<nc::NdArray<Complex>>::from_json(const json&, nc::NdArray<Complex>&);
    template void adl_serializer<nc::NdArray<Complex>>::from_json(const ordered_json&, nc::NdArray<Complex>&);

    // Explicit Instantiations for nc::NdArray<nc::Vec2>
    template struct adl_serializer<nc::NdArray<nc::Vec2>>;
    template void adl_serializer<nc::NdArray<nc::Vec2>>::to_json(json&, const nc::NdArray<nc::Vec2>&);
    template void adl_serializer<nc::NdArray<nc::Vec2>>::to_json(ordered_json&, const nc::NdArray<nc::Vec2>&);
    template void adl_serializer<nc::NdArray<nc::Vec2>>::from_json(const json&, nc::NdArray<nc::Vec2>&);
    template void adl_serializer<nc::NdArray<nc::Vec2>>::from_json(const ordered_json&, nc::NdArray<nc::Vec2>&);

    // Explicit Instantiations for nc::NdArray<nc::Vec3>
    template struct adl_serializer<nc::NdArray<nc::Vec3>>;
    template void adl_serializer<nc::NdArray<nc::Vec3>>::to_json(json&, const nc::NdArray<nc::Vec3>&);
    template void adl_serializer<nc::NdArray<nc::Vec3>>::to_json(ordered_json&, const nc::NdArray<nc::Vec3>&);
    template void adl_serializer<nc::NdArray<nc::Vec3>>::from_json(const json&, nc::NdArray<nc::Vec3>&);
    template void adl_serializer<nc::NdArray<nc::Vec3>>::from_json(const ordered_json&, nc::NdArray<nc::Vec3>&);
} // namespace nlohmann
