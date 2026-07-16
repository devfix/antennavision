//
// Created by core on 2026-07-16.
//

#include "serialization.hpp"
#include <algorithm>
#include <nlohmann/json.hpp>

namespace serialization
{
    namespace
    {
        template <typename T>
        void assert_field(const T& j, std::string_view structure_name, const char* name, std::uint8_t expected_type_int, bool mandatory)
        {
            auto expected_type = static_cast<nlohmann::json::value_t>(expected_type_int);

            if (!j.contains(name))
            {
                if (mandatory) { throw SimulationError("Failed to load {} from JSON: missing mandatory field '{}'", structure_name, name); }
                return;
            }

            if (j.at(name).type() != expected_type)
            {
                throw SimulationError("Failed to load {} from JSON, expected type '{}' for field '{}' but got '{}'", structure_name, nlohmann::json(expected_type).type_name(), name,
                                      j.at(name).type_name());
            }
        }
    } // namespace

    template <any_json_t JsonType>
    void assert_structure(JsonType const& js, std::string_view structure_name, std::vector<JsonField> const& mandatory, std::vector<JsonField> const& optional)
    {
        if (!js.is_object()) { throw SimulationError("Failed to load {} from JSON, expected JSON object, got {}", structure_name, js.type_name()); }
        for (auto const& field : mandatory) { assert_field(js, structure_name, field.name, field.type, true); }
        for (auto const& field : optional) { assert_field(js, structure_name, field.name, field.type, false); }
        for (auto const& [key, value] : js.items())
        {
            if (!std::ranges::contains(mandatory, key, &JsonField::name) && !std::ranges::contains(optional, value, &JsonField::name))
            {
                throw SimulationError("Failed to load {} from JSON, invalid JSON entry '{}' of type '{}' found", structure_name, key, value.type_name());
            }
        }
    }

    template void assert_structure(json const& j, std::string_view structure_name, std::vector<JsonField> const& mandatory, std::vector<JsonField> const& optional);
    template void assert_structure(ojson const& j, std::string_view structure_name, std::vector<JsonField> const& mandatory, std::vector<JsonField> const& optional);
} // namespace serialization
