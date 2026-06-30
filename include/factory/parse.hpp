//
// Created by core on 18.06.26.
//

#pragma once

#include <functional>
#include <map>
#include <string>

namespace factory
{
    std::function<std::complex<double>(double polar, double azimuth, double wavelength)> parse_polar_azimuth_function(std::string const& expr);

    double parse_double(std::string const& expr, std::map<std::string, double> const& variables);
} // namespace factory
