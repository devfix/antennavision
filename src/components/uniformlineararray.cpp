//
// Created by core on 2026-07-12.
//

#include "components/uniformlineararray.hpp"

UniformLinearArray::UniformLinearArray(std::string_view const id, Reference& origin, std::list<Radiator>&& elements, std::vector<complex_t>&& coeffs) :
    RadiatorArray(id, origin, std::move(elements), std::move(coeffs))
{}

