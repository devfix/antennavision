//
// Created by core on 2026-07-12.
//

#include "components/uniformplanararray.hpp"
#include "simulationerror.hpp"
//
// UniformPlanarArray::UniformPlanarArray(std::string_view const id, std::string const& origin, std::vector<Radiator>&& elements, std::vector<complex_t>&& coeffs, std::size_t size_x,
//                                        std::size_t size_y) : RadiatorArray(id, origin, std::move(elements), std::move(coeffs)), size_x_(size_x), size_y_(size_y)
// {
//     if (size != size_x_ * size_y_)
//     {
//         throw SimulationError("Number of UPA elements should be 'size = length_x * length_y' but is size={} length_x={} length_y={}", size, size_x, size_y);
//     }
// }
