//
// Created by core on 2026-07-08.
//

#pragma once

#include "components/radiatorarray.hpp"

struct UniformLinearArray : RadiatorArray<UniformLinearArray>
{
    UniformLinearArray(std::string_view id, Reference& origin, std::list<Radiator>&& elements);
    constexpr Reference& get_reference(std::size_t const idx) const { return (*this)(idx).origin; }
};
