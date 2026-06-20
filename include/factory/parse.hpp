//
// Created by core on 18.06.26.
//

#pragma once

#include <string>
#include <map>
#include <functional>

namespace factory
{
    std::function<double(double, double)> parse_theta_phi_function(std::string const& expr);

    double parse_double(std::string const& expr, std::map<std::string, double> const& variables);
} // namespace factory
