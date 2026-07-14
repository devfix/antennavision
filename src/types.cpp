//
// Created by core on 2026-07-14.
//

#include "types.hpp"
#include <nlohmann/json.hpp>


namespace nc {
    template <typename BasicJsonType>
    void to_json(BasicJsonType& j, Vec3 const& v) {
        j = nlohmann::json{ v.x, v.y, v.z };
    }

    template <typename BasicJsonType>
    void from_json(BasicJsonType const& j, Vec3& v) {
        // If the JSON is a 3-element array [x, y, z]
        if (j.is_array() && j.size() == 3) {
            v.x = j.at(0).template get<double>();
            v.y = j.at(1).template get<double>();
            v.z = j.at(2).template get<double>();
        } else {
            throw nlohmann::json::type_error::create(302, "Validation failed: expected a 3-element array for nc::Vec3", &j);
        }
    }

    template <typename BasicJsonType>
    void to_json(BasicJsonType& j, Vec2 const& v) {
        j = nlohmann::json{ v.x, v.y};
    }

    template <typename BasicJsonType>
    void from_json(BasicJsonType const& j, Vec2& v) {
        // If the JSON is a 2-element array [x, y]
        if (j.is_array() && j.size() == 2) {
            v.x = j.at(0).template get<double>();
            v.y = j.at(1).template get<double>();
        } else {
            throw nlohmann::json::type_error::create(302, "Validation failed: expected a 2-element array for nc::Vec2", &j);
        }
    }
} // namespace nc

// nc::Vec2 Instantiations
template void nc::to_json(nlohmann::json&, nc::Vec2 const&);
template void nc::to_json(nlohmann::ordered_json&, nc::Vec2 const&);
template void nc::from_json(nlohmann::json const&, nc::Vec2&);
template void nc::from_json(nlohmann::ordered_json const&, nc::Vec2&);

// nc::Vec3 Instantiations
template void nc::to_json(nlohmann::json&, nc::Vec3 const&);
template void nc::to_json(nlohmann::ordered_json&, nc::Vec3 const&);
template void nc::from_json(nlohmann::json const&, nc::Vec3&);
template void nc::from_json(nlohmann::ordered_json const&, nc::Vec3&);