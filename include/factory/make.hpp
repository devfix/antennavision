//
// Created by core on 18.06.26.
//

#pragma once

#include <map>
#include "components/antenna.hpp"
#include "components/radiator.hpp"

namespace factory
{
    using task_t = std::function<void()>;
    struct Context
    {
        ojson &desc;
        std::map<std::string, double> variables;
        std::list<Reference> references;
        std::map<std::string, Antenna> antennas;
        std::list<std::pair<std::string, task_t>> tasks;
    };

    Reference& make_reference(ojson& reference_desc, std::list<Reference>& references, std::map<std::string, double> const& variables);
    [[nodiscard]] Antenna make_antenna(ojson& desc, Context& context);
} // namespace factory
