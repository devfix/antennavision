//
// Created by core on 30.06.26.
//

#pragma once

#include "components/radiator.hpp"
#include "ulacodebook.hpp"

// CRTP scheme
template <typename Derived>
struct RadiatorArray
{
    RadiatorArray(std::string_view const id, Reference& origin, std::list<Radiator>&& elements, std::vector<complex_t>&& coeffs) :
        id(id), origin(origin), size(elements.size()), elements(std::move(elements)), element_lookup(size, nullptr), coeffs(std::move(coeffs))
    {
        // important: use this->elements since elements is already moved and its size zero
        for (std::size_t idx = 0; auto& element : this->elements) { element_lookup[idx++] = &element; }
    }

    constexpr Radiator& operator()(std::size_t const idx) const { return *element_lookup[idx]; }

    // the member variables must be non-const, otherwise the object cannot be moved which is required for returning a std::variant
    std::string const id;
    Reference& origin;
    std::size_t const size;
    std::list<Radiator> elements;
    std::vector<Radiator*> element_lookup;
    std::vector<complex_t> coeffs;
};
