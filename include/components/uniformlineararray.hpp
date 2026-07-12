//
// Created by core on 2026-07-08.
//

#pragma once

#include "components/radiatorarray.hpp"

struct UniformLinearArray : RadiatorArray<UniformLinearArray>
{
    UniformLinearArray(std::string_view id, Reference& origin, std::list<Radiator>&& elements);
};
