//
// Created by core on 2026-07-08.
//

#pragma once

#include "components/radiatorarray.hpp"

struct UniformLinearArray : RadiatorArray<UniformLinearArray>
{
    UniformLinearArray(std::string_view const id, std::list<Reference>&& references, std::vector<std::unique_ptr<Radiator>>&& radiators) :
        RadiatorArray(id, std::move(references), std::move(radiators))
    {}
};
