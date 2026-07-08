//
// Created by core on 30.06.26.
//

#pragma once

#include <memory>
#include "components/radiator.hpp"
#include "ulacodebook.hpp"

// CRTP scheme
template<typename Derived>
struct RadiatorArray
{
    RadiatorArray(std::string_view const id, std::list<Reference> && references, std::vector<std::unique_ptr<Radiator>> && radiators)
        : id(id), references(std::move(references)), radiators(std::move(radiators))
    {}

    // the member variables must be non-const, otherwise the object cannot be moved which is required for returning a std::variant
    std::string id;
    std::list<Reference> references;
    std::vector<std::unique_ptr<Radiator>> radiators;
};