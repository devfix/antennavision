//
// Created by Tristan Krause on 2026-05-26.
//

#include "components/source.hpp"

Source::Source(std::string_view const id, double const power_dBm) : Component(id, 0, 1), power_dBm(power_dBm) {}
