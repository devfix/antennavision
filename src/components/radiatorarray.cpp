//
// Created by core on 30.06.26.
//

#include "components/radiatorarray.hpp"

RadiatorArray::RadiatorArray(std::string_view const id, std::vector<std::reference_wrapper<Radiator>> const& elements) :
    id(id), elements(elements)
{}
