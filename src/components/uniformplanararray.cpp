//
// Created by core on 2026-07-12.
//

#include "components/uniformplanararray.hpp"

#include "simulationerror.hpp"

UniformPlanarArray::UniformPlanarArray(std::string_view const id, Reference& origin, std::list<Radiator>&& elements, std::size_t size_x,
                                       std::size_t size_y) : RadiatorArray(id, origin, std::move(elements)), size_x(size_x), size_y(size_y)
{
    if (size != size_x * size_y)
    {
        throw SimulationError("Number of UPA elements should be 'size = length_x * length_y' but is size={} length_x={} length_y={}", size, size_x, size_y);
    }
}
