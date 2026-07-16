//
// Created by core on 2026-07-14.
//

#include "types.hpp"
#include <nlohmann/json.hpp>

namespace nlohmann {
    // ====================================================================================
    // std::complex Serializer
    // ====================================================================================
    template<typename T>
    template<any_json_t JsonType>
    void adl_serializer<std::complex<T>>::to_json(JsonType& js, const std::complex<T>& value) {
        js = JsonType{ value.real(), value.imag() };
    }
    template<typename T>
    template<any_json_t JsonType>
    void adl_serializer<std::complex<T>>::from_json(const JsonType& j, std::complex<T>& value) {
        if (j.is_array() && j.size() == 2) {
            value.real(j.at(0).template get<T>());
            value.imag(j.at(1).template get<T>());
        } else {
            throw JsonType::type_error::create(302, "Expected a 2-element array for std::complex", &j);
        }
    }

    // Instantiations for std::complex<std::float64_t>
    template void adl_serializer<complex_t>::to_json(json&, complex_t const&);
    template void adl_serializer<complex_t>::to_json(ordered_json&, complex_t const&);
    template void adl_serializer<complex_t>::from_json(json const&, complex_t&);
    template void adl_serializer<complex_t>::from_json(ordered_json const&, complex_t&);

    // ====================================================================================
    // nc::Vec2 Serializer
    // ====================================================================================
    template<any_json_t JsonType>
    void adl_serializer<nc::Vec2>::to_json(JsonType& js, const nc::Vec2& value) {
        js = JsonType{ value.x, value.y };
    }
    template<any_json_t JsonType>
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

    // ====================================================================================
    // nc::Vec3 Serializer
    // ====================================================================================
    template<any_json_t JsonType>
    void adl_serializer<nc::Vec3>::to_json(JsonType& js, const nc::Vec3& value) {
        js = JsonType{ value.x, value.y, value.z };
    }
    template<any_json_t JsonType>
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

    // ====================================================================================
    // nc::rotations::Quaternion Serializer
    // ====================================================================================
    template<any_json_t JsonType>
    void adl_serializer<Quaternion>::to_json(JsonType& js, const Quaternion& value) {
        // Always serialize as a 4-element array [s, i, j, k]
        js = JsonType{ value.s(), value.i(), value.j(), value.k() };
    }

    template<any_json_t JsonType>
    void adl_serializer<Quaternion>::from_json(const JsonType& js, Quaternion& value)
    {
        if (js.is_array() && js.size() == 4) {
            // Case 1: Read raw 4-element quaternion representation
            std::float64_t const cs = js.at(0).template get<std::float64_t>();
            std::float64_t const ci = js.at(1).template get<std::float64_t>();
            std::float64_t const cj = js.at(2).template get<std::float64_t>();
            std::float64_t const ck = js.at(3).template get<std::float64_t>();
            value = Quaternion(ci, cj, ck, cs);
        }
        else if (js.is_object() && js.contains("roll") && js.contains("pitch") && js.contains("yaw")) {
            // Case 2: Read fallback Roll-Pitch-Yaw object representation
            std::float64_t const roll = js.at("roll").template get<std::float64_t>();
            std::float64_t const pitch = js.at("pitch").template get<std::float64_t>();
            std::float64_t const yaw = js.at("yaw").template get<std::float64_t>();
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

    // ====================================================================================
    // nc::NdArray<T> Serializer
    // ====================================================================================
    template<typename T>
    template<any_json_t JsonType>
    void adl_serializer<nc::NdArray<T>>::to_json(JsonType& js, const nc::NdArray<T>& value) {
        auto shape = value.shape();
        js = JsonType::array();

        for (std::uint32_t row = 0; row < shape.rows; ++row) {
            JsonType inner_row = JsonType::array();
            for (std::uint32_t col = 0; col < shape.cols; ++col) {
                // This will automatically find the correct ADL serializer
                // for the element type T (e.g., complex_t or std::float64_t)
                inner_row.push_back(value(row, col));
            }
            js.push_back(inner_row);
        }
    }

    template<typename T>
    template<any_json_t JsonType>
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

    // Explicit Instantiations for nc::NdArray<std::float64_t>
    template struct adl_serializer<nc::NdArray<std::float64_t>>;
    template void adl_serializer<nc::NdArray<std::float64_t>>::to_json(json&, const nc::NdArray<std::float64_t>&);
    template void adl_serializer<nc::NdArray<std::float64_t>>::to_json(ordered_json&, const nc::NdArray<std::float64_t>&);
    template void adl_serializer<nc::NdArray<std::float64_t>>::from_json(const json&, nc::NdArray<std::float64_t>&);
    template void adl_serializer<nc::NdArray<std::float64_t>>::from_json(const ordered_json&, nc::NdArray<std::float64_t>&);

    // Explicit Instantiations for nc::NdArray<complex_t>
    template struct adl_serializer<nc::NdArray<complex_t>>;
    template void adl_serializer<nc::NdArray<complex_t>>::to_json(json&, const nc::NdArray<complex_t>&);
    template void adl_serializer<nc::NdArray<complex_t>>::to_json(ordered_json&, const nc::NdArray<complex_t>&);
    template void adl_serializer<nc::NdArray<complex_t>>::from_json(const json&, nc::NdArray<complex_t>&);
    template void adl_serializer<nc::NdArray<complex_t>>::from_json(const ordered_json&, nc::NdArray<complex_t>&);

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
