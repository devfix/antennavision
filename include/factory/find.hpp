//
// Created by Tristan Krause on 2026-06-18.
//

#pragma once

#include <format>
#include "components/antenna.hpp"
#include "reference.hpp"

namespace factory
{
    template <typename ContainerType>
    auto& find_reference_by_id(ContainerType&& references, std::string_view const id)
    {
        if (const auto it = std::ranges::find_if(references, [id](Reference const& reference) { return reference.id == id; }); it != references.end())
        {
            return *it;
        }
        throw std::runtime_error(std::format("Error: Could not find reference with id '{}'", id));
    }

    template <typename ContainerType>
    auto& find_radiator_by_id(ContainerType&& radiators, std::string_view const id)
    {
        if (auto const it = std::ranges::find_if(radiators, [id](auto const& radiator) { return antenna::get_id(radiator) == id; });
            it != radiators.end())
        {
            return **it;
        }
        throw std::runtime_error(std::format("Error: Could not find radiator with id '{}'", id));
    }
} // namespace factory
