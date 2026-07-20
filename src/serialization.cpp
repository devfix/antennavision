//
// Created by core on 2026-07-16.
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
                throw SimulationError("Failed to load {} from JSON, expected type '{}' for field '{}' but got '{}'", structure_name,
                    nlohmann::json(expected_type).type_name(), name, j.at(name).type_name());
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
            if (!std::ranges::contains(mandatory, key, &JsonField::name) and !std::ranges::contains(optional, key, &JsonField::name))
            {
                throw SimulationError("Failed to load {} from JSON, invalid JSON entry '{}' of type '{}' found", structure_name, key, value.type_name());
            }
        }
    }

    template void assert_structure(
        json const& j, std::string_view structure_name, std::vector<JsonField> const& mandatory, std::vector<JsonField> const& optional);
    template void assert_structure(
        ojson const& j, std::string_view structure_name, std::vector<JsonField> const& mandatory, std::vector<JsonField> const& optional);
} // namespace serialization

template <any_json_t JsonType>
void to_json(JsonType& js, Reference const& ref)
{
    js = JsonType{
        {"id", ref.id},
        {"origin", ref.origin ? ref.origin->id : ""},
        {"pos", ref.pos},
        {"rot", ref.rot},
    };
}

template <any_json_t JsonType>
void from_json(JsonType const& js, Reference& ref)
{
    serialization::assert_structure(js, "Reference",
        {
            {"id", json::value_t::string},
            {"origin", json::value_t::string},
        },
        {
            {"pos", json::value_t::array},
            {"rot", json::value_t::object},
        });
    std::string id;
    std::string origin;
    pos_t pos;
    Quaternion rot;

    js.at("id").get_to(id);
    js.at("origin").get_to(origin);
    if (js.contains("pos")) { js.at("pos").get_to(pos); }
    if (js.contains("rot")) { js.at("rot").get_to(rot); }

    ref = Reference{.id = id, .origin_id = origin, .pos = pos, .rot = rot};
}

// Reference Instantiations
template void to_json(nlohmann::json&, Reference const&);
template void to_json(nlohmann::ordered_json&, Reference const&);
template void from_json(nlohmann::json const&, Reference&);
template void from_json(nlohmann::ordered_json const&, Reference&);
