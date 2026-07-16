//
// Created by core on 2026-07-14.
//

#include "types.hpp"
#include <nlohmann/json.hpp>

namespace nlohmann {
    // ==========================================
    // std::complex Serializer
    // ==========================================
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

    // Instantiations for std::complex<double>
    template void adl_serializer<complex_t>::to_json(json&, complex_t const&);
    template void adl_serializer<complex_t>::to_json(ordered_json&, complex_t const&);
    template void adl_serializer<complex_t>::from_json(json const&, complex_t&);
    template void adl_serializer<complex_t>::from_json(ordered_json const&, complex_t&);

    // ==========================================
    // nc::Vec2 Serializer
    // ==========================================
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

    // ==========================================
    // nc::Vec3 Serializer
    // ==========================================
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

    // ==========================================
    // nc::rotations::Quaternion Serializer
    // ==========================================
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
            double cs = js.at(0).template get<double>();
            double ci = js.at(1).template get<double>();
            double cj = js.at(2).template get<double>();
            double ck = js.at(3).template get<double>();
            value = Quaternion(ci, cj, ck, cs);
        }
        else if (js.is_object() && js.contains("roll") && js.contains("pitch") && js.contains("yaw")) {
            // Case 2: Read fallback Roll-Pitch-Yaw object representation
            double roll = js.at("roll").template get<double>();
            double pitch = js.at("pitch").template get<double>();
            double yaw = js.at("yaw").template get<double>();
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
} // namespace nlohmann

namespace nc
{
    template <typename...>
    inline constexpr bool always_false = false;

    template <any_json_t JsonType, typename T>
    void to_json(JsonType& j, NdArray<T> const& array)
    {
        auto shape = array.shape();
        JsonType outer_array = JsonType::array();

        // Loop through rows and columns to form a nested 2D JSON array
        for (uint32 row = 0; row < shape.rows; ++row)
        {
            JsonType inner_row = JsonType::array();
            for (uint32 col = 0; col < shape.cols; ++col)
            {
                if constexpr (std::is_same_v<std::decay_t<T>, complex_t>)
                {
                    auto const& v = array(row, col);
                    inner_row.push_back({std::abs(v), std::arg(v)});
                }
                else
                {
                    static_assert(always_false<T>, "Unsupported element type of nc::NdArray");
                }
            }
            outer_array.push_back(inner_row);
        }
        j = outer_array;
    }

    template <any_json_t JsonType, typename T>
    void from_json(JsonType const& j, NdArray<T>& array)
    {}
} // namespace nc

// nc::NdArray<complex_t>
template void nc::to_json(nlohmann::json&, nc::NdArray<complex_t> const&);
template void nc::to_json(nlohmann::ordered_json&, nc::NdArray<complex_t> const&);
template void nc::from_json(nlohmann::json const&, nc::NdArray<complex_t>&);
template void nc::from_json(nlohmann::ordered_json const&, nc::NdArray<complex_t>&);
