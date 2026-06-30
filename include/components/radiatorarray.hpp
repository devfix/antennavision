//
// Created by core on 30.06.26.
//

#pragma once

#include <functional>
#include "components/radiator.hpp"

struct RadiatorArray
{
    RadiatorArray(std::string_view id, std::vector<std::reference_wrapper<Radiator>> const& elements);

    std::string id;
    std::vector<std::reference_wrapper<Radiator>> elements;
};
