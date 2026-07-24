//
// Created by Tristan Krause on 2026-07-16.
//

#pragma once

#include <string_view>
#include "reference.hpp"
#include "simulationerror.hpp"
#include "types/json.hpp"

namespace serialization
{

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

    template <any_json_t JsonType>
    void assert_structure(JsonType const& js, std::string_view structure_name, std::vector<JsonField> const& mandatory, std::vector<JsonField> const& optional);
} // namespace serialization

template <any_json_t JsonType>
void to_json(JsonType& j, Reference const& ref);

template <any_json_t JsonType>
void from_json(JsonType const& j, Reference& ref);
