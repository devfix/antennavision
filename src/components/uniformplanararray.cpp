//
// Created by core on 2026-07-12.
//

#include "components/uniformplanararray.hpp"

UniformPlanarArray::UniformPlanarArray(std::string_view const id, Reference& origin, std::list<Radiator>&& elements) :
        RadiatorArray(id, origin, std::move(elements))
{}