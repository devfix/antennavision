//
// Created by Tristan Krause on 2026-06-18.
//

#pragma once

#include <map>
#include <variant>
#include "../setup/geometry.hpp"
#include "components/antenna.hpp"
#include "components/radiator.hpp"

namespace factory
{
    using task_t = std::function<void()>;
    struct Context
    {
        ojson &desc;
        std::map<std::string, var_t> variables;
        setup::NumParams num_params;
        std::vector<Reference> references;
        std::vector<Antenna> antennas;
        std::vector<geometry::Geometry> geometries;
        std::list<std::pair<std::string, task_t>> tasks;
    };

    [[nodiscard]] Reference make_reference(ojson& desc, Context const& context);
    [[nodiscard]] Antenna make_antenna(ojson& desc, Context& context);
    [[nodiscard]] geometry::Geometry make_geometry(ojson& desc, Context& context);
} // namespace factory
