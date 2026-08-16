//
// Created by Tristan Krause on 2026-08-16.
//

#pragma once
#include "components/antenna.hpp"
#include "reference.hpp"
#include "setup/geometry.hpp"
#include "setup/sweep.hpp"
#include "types/setup.hpp"

struct Context
{
    std::span<Codebook const> codebooks;
    VarMap const& variables;
    std::span<reference::Reference const> references;
    std::span<antenna::Antenna const> antennas;
    std::span<geometry::Geometry const> geometries;
    std::span<sweep::Sweep const> sweeps;
};
