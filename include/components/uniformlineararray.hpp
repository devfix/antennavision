//
// Created by core on 2026-07-08.
//

#pragma once

#include "components/radiatorarray.hpp"

struct UniformLinearArray : RadiatorArray<UniformLinearArray>
{
    UniformLinearArray(std::string_view id, Reference& origin, std::list<Radiator>&& elements);

    Radiator& get_element(std::size_t idx);
    Reference& get_reference(std::size_t idx);
};
