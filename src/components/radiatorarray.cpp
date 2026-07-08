//
// Created by core on 30.06.26.
//

#include "components/radiatorarray.hpp"

RadiatorArray::RadiatorArray(std::string_view const id, std::vector<std::reference_wrapper<Radiator>> const& elements, std::optional<std::filesystem::path> path_codebook) :
    id(id), elements(elements), ula_codebook(path_codebook->empty() ? std::nullopt : decltype(ula_codebook)(path_codebook.value()))
{}
