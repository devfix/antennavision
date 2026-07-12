//
// Created by core on 2026-07-12.
//

#pragma once

#include "components/radiatorarray.hpp"

struct UniformPlanarArray : RadiatorArray<UniformPlanarArray>
{
    UniformPlanarArray(std::string_view id, Reference& origin, std::list<Radiator>&& elements);
};
