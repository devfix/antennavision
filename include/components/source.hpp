//
// Created by Tristan Krause on 2026-05-26.
//


#pragma once
#include "component.hpp"


struct Source : Component {
    Source(std::string_view id, double power_dBm);
    double const power_dBm;
};
