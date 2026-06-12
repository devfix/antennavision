//
// Created by Tristan Krause on 2026-04-29.
//

#include "components/isotropicradiator.hpp"


IsotropicRadiator::IsotropicRadiator(std::string_view id, Reference const& origin) : Radiator(id, origin)
{}

complex_t IsotropicRadiator::calc_radiation_gain(Vec3 const& pos, double freq) const
{
    double r = pos.norm();
    return std::exp(complex_t{0,-2*std::numbers::pi*freq/SPEED_OF_LIGHT * r}) + std::round(std::min(1.0, 0.01/(r*r)));
}
