//
// Created by core on 18.06.26.
//

#pragma once

#include <string_view>
#include <functional>

namespace factory
{
    std::function<double(double, double)> parse_theta_phi_function(std::string_view expression);
} // namespace factory
