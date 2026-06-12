//
// Created by Tristan Krause on 2026-06-08.
//

#include "components/customradiator.hpp"

CustomRadiator::CustomRadiator(std::string_view id, Reference const &origin, std::function<Vec3(double, double)>&& effective_length) : Radiator(id, origin), effective_length(std::move(effective_length)) {}

Vec3 CustomRadiator::calc_el_polar(double const theta, double const phi) const { return effective_length(theta, phi); }

complex_t CustomRadiator::calc_radiation_gain(Vec3 const &pos, double freq) const
{
    throw std::runtime_error("not implemented");
}
