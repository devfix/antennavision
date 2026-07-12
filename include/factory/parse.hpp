//
// Created by core on 18.06.26.
//

#pragma once

#include <functional>
#include <map>
#include <string>
#include "types.hpp"

namespace factory
{
    std::function<complex_t(double polar, double azimuth, double wavelength)> parse_polar_azimuth_function(std::string const& expr);

    double parse_double(std::string const& expr, std::map<std::string, double> const& variables);
} // namespace factory
