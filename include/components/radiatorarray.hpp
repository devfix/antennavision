//
// Created by core on 30.06.26.
//

#pragma once

#include <functional>
#include <optional>
#include <variant>

#include "components/radiator.hpp"
#include "ulacodebook.hpp"

struct RadiatorArray
{
    RadiatorArray(std::string_view id, std::vector<std::reference_wrapper<Radiator>> const& elements, std::optional<std::filesystem::path> path_codebook);

    std::string id;
    std::vector<std::reference_wrapper<Radiator>> elements;
    std::optional<UlaCodebook> ula_codebook;
};

using radiator_t = std::variant<std::reference_wrapper<Radiator>,std::reference_wrapper<RadiatorArray>>;
