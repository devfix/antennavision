//
// Created by Tristan Krause on 2026-07-12.
//

#pragma once

#include "components/radiatorarray.hpp"

/**
 * Class "UniformPlanarArray" of Aggregate OutputType
 * Also known as POD (Plain Old Data) / PDS (Passive Data Structure) / DTO (Data Transfer Object)
 */
struct UniformPlanarArray : RadiatorArray<UniformPlanarArray>
{
    constexpr Radiator& operator()(std::size_t x, std::size_t y) { return elements.at(y * size_x + x); }

    reference::Reference& get_reference(std::size_t x, std::size_t y) { auto const ptr = (*this)(x, y).origin;
        if (!ptr) { throw SimulationError("Element {}:{} UniformPlanarArray '{}' has unconfigured origin", x, y, id); }
        return *ptr;
    }

    std::size_t size_x{};
    std::size_t size_y{};
};
