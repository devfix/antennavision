//
// Created by core on 30.06.26.
//

#pragma once

#include "components/radiator.hpp"
#include "ulacodebook.hpp"

// CRTP scheme
template <typename Derived>
struct RadiatorArray
{
    RadiatorArray(std::string_view const id, Reference& origin, std::list<Radiator>&& elements) :
        id(id), origin(origin), elements(std::move(elements))
    {}

    // the member variables must be non-const, otherwise the object cannot be moved which is required for returning a std::variant
    std::string id;
    Reference& origin;
    std::list<Radiator> elements;
};
