//
// Created by Tristan Krause on 2026-06-18.
//

#pragma once

#include <variant>
#include "types/setup.hpp"
#include "components/antenna.hpp"
#include "setup/geometry.hpp"
#include "setup/sweep.hpp"

namespace factory
{
    [[nodiscard]] reference::Reference make_reference(ojson& desc, VarMap const& variables);
    [[nodiscard]] antenna::Antenna make_antenna(ojson& desc, VarMap const& variables);
    [[nodiscard]] geometry::Geometry make_geometry(ojson& desc, VarMap const& variables);
    [[nodiscard]] sweep::Sweep make_sweep(ojson& desc, VarMap const& variables);
} // namespace factory
