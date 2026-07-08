//
// Created by core on 2026-07-08.
//

#pragma once

#include <variant>
#include "components/uniformlineararray.hpp"

struct UniformLinearArray; // forward declaration
// using GenericRadiatorArray = std::variant<UniformLinearArray>;
using Antenna = std::variant<Radiator, UniformLinearArray>;

namespace antenna
{
    constexpr std::string_view get_id(Antenna const& antenna)
    {
        return std::visit([](auto const& ant) { return ant.id; }, antenna);
    }
} // namespace antenna
