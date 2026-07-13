//
// Created by core on 2026-07-12.
//

#include "components/uniformlineararray.hpp"

UniformLinearArray::UniformLinearArray(std::string_view const id, Reference& origin, std::list<Radiator>&& elements) :
        RadiatorArray(id, origin, std::move(elements))
{}

Radiator& UniformLinearArray::get_element(std::size_t const idx)
{
    auto it = elements.begin();
    std::advance(it, idx);
    return *it;
}

Reference& UniformLinearArray::get_reference(std::size_t const idx)
{
    return get_element(idx).origin;
}
