//
// Created by core on 18.06.26.
//

#pragma once

#include <map>
#include <memory>
#include "components/radiator.hpp"

namespace factory
{
    Reference& make_reference(json& reference_desc, std::list<Reference>& references, std::map<std::string, double> const& variables);
    void make_radiator(json& radiator_desc, std::list<Reference>& references, std::list<std::unique_ptr<Radiator>>& radiators, std::map<std::string, double> const& variables, bool generate);
} // namespace factory
