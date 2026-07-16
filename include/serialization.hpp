//
// Created by core on 2026-07-16.
//

#pragma once

#include "types.hpp"
#include <string_view>
#include "simulationerror.hpp"

namespace serialization
{
    struct JsonField
    {
        template <typename T>
        constexpr JsonField(char const* name, T type) : name(name), type(static_cast<std::uint8_t>(type))
        {}

        const char* name;
        std::uint8_t type;
    };

    template <any_json_t JsonType>
    void assert_structure(JsonType const& js, std::string_view structure_name, std::vector<JsonField> const& mandatory, std::vector<JsonField> const& optional);
} // namespace serialization
