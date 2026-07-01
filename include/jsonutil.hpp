//
// Created by core on 01.07.26.
//

#pragma once

#include <nlohmann/json.hpp>
#include "types.hpp"

// 1. Tell nlohmann how to serialize/deserialize std::complex
namespace std {
    template<typename T>
    void to_json(json& j, const std::complex<T>& c) {
        // Storing as an array: [real, imag]
        j = json{ c.real(), c.imag() };
    }

    template<typename T>
    void from_json(const json& j, std::complex<T>& c) {
        c.real(j.at(0).get<T>());
        c.imag(j.at(1).get<T>());
    }
}

// 2. Tell nlohmann how to serialize/deserialize nc::NdArray
namespace nc {
    inline void to_json(json& j, const Vec3& v) {
        j = json{ v.x, v.y, v.z };
    }

    inline void from_json(const json& j, Vec3& v) {
        v.x = j.at(0).get<double>();
        v.y = j.at(1).get<double>();
        v.z = j.at(2).get<double>();
    }

    template<typename T>
    void to_json(json& j, const NdArray<T>& array) {
        auto shape = array.shape();
        json outer_array = json::array();

        // Loop through rows and columns to form a nested 2D JSON array
        for (nc::uint32 row = 0; row < shape.rows; ++row) {
            json inner_row = json::array();
            for (nc::uint32 col = 0; col < shape.cols; ++col) {
                inner_row.push_back(array(row, col));
            }
            outer_array.push_back(inner_row);
        }
        j = outer_array;
    }

    template<typename T>
    void from_json(const json& j, NdArray<T>& array) {
        nc::uint32 rows = j.size();
        nc::uint32 cols = rows > 0 ? j.at(0).size() : 0;

        // Resize or create the array with the appropriate dimensions
        array = NdArray<T>(rows, cols);

        for (nc::uint32 row = 0; row < rows; ++row) {
            for (nc::uint32 col = 0; col < cols; ++col) {
                array(row, col) = j.at(row).at(col).get<T>();
            }
        }
    }

}
