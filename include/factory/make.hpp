//
// Created by core on 18.06.26.
//

#pragma once

#include <map>
#include <memory>
#include "components/radiator.hpp"

namespace factory
{
    Reference make_reference(json &reference_desc, std::list<Reference> const &references, std::map<std::string, double> const& variables);
    std::unique_ptr<Radiator> make_radiator(json &radiator_desc, std::list<Reference> const &references, std::map<std::string, double> const& variables);
} // namespace factory
