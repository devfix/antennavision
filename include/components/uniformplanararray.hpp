//
// Created by core on 2026-07-12.
//

#pragma once

#include "components/radiatorarray.hpp"

struct UniformPlanarArray : RadiatorArray<UniformPlanarArray>
{
    UniformPlanarArray(std::string_view id, Reference& origin, std::list<Radiator>&& elements, std::vector<complex_t>&& coeffs, std::size_t size_x, std::size_t size_y);

    constexpr Radiator& operator()(std::size_t const x, std::size_t const y) const { return *element_lookup[y * size_x + x]; }

    constexpr Reference& get_reference(std::size_t const x, std::size_t const y) const { return (*this)(x, y).origin; }

    std::size_t const size_x;
    std::size_t const size_y;
};
