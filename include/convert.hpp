//
// Created by Tristan Krause on 2026-08-07.
//

#pragma once

#include <string_view>

namespace convert
{
    [[nodiscard]] int int_from_string(std::string_view str);
    [[nodiscard]] std::string string_from_version(std::array<int, 3> const& version);
} // namespace convert