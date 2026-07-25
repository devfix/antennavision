//
// Created by Tristan Krause on 2026-06-30.
//

#pragma once

#include "components/radiator.hpp"
#include "ulacodebook.hpp"

/**
 * @brief Base class implementing the Curiously Recurring Template Pattern (CRTP). The derived classes are of Aggregate Type.
 * @tparam Derived The derived class extending this CRTP base.
 */
template <typename Derived>
struct RadiatorArray
{
    // RadiatorArray(std::string_view const id, std::string const& origin_id, std::vector<Radiator>&& elements, std::vector<complex_t>&& coefficients) :
    //     id(id), origin_id(origin_id), size(elements.size()), elements(std::move(elements)), coefficients(std::move(coefficients))
    // {}

    std::string id;
    std::string origin_id;
    std::vector<Radiator> elements;
    std::vector<complex_t> coefficients;
    Reference* origin{};
};
