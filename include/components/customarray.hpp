//
// Created by Tristan Krause on 2026-07-08.
//

#pragma once

#include "components/radiatorarray.hpp"

/**
 * Class "CustomArray" of Aggregate OutputType
 * Also known as POD (Plain Old Data) / PDS (Passive Data Structure) / DTO (Data Transfer Object)
 */
struct CustomArray : RadiatorArray<CustomArray>
{
    constexpr Radiator& operator()(std::size_t idx) { return elements.at(idx); }

    reference::Reference const& get_reference(std::size_t idx) const
    {
        auto const ptr = elements.at(idx).origin;
        if (!ptr) { throw SimulationError("Element {} CustomArray '{}' has unconfigured origin", idx, id); }
        return *ptr;
    }
};
