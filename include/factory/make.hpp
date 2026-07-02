//
// Created by core on 18.06.26.
//

#pragma once

#include <map>
#include <memory>
#include "components/radiator.hpp"
#include "components/radiatorarray.hpp"

namespace factory
{
    Reference& make_reference(ojson& reference_desc, std::list<Reference>& references, std::map<std::string, double> const& variables);
    std::vector<std::reference_wrapper<Radiator>> make_radiator(ojson& radiator_desc, std::list<Reference>& references, std::list<std::unique_ptr<Radiator>>& radiators, std::map<std::string, double> const& variables, std::map<std::string, RadiatorArray> & radiator_arrays, bool generate);
} // namespace factory
