//
// Created by core on 18.06.26.
//

#pragma once

#include <memory>
#include "components/radiator.hpp"

namespace factory
{
    Reference make_reference(json &reference_desc, std::list<Reference> const &references);
    std::unique_ptr<Radiator> make_radiator(json &radiator_desc, std::list<Reference> const &references);
} // namespace factory
