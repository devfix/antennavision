//
// Created by Tristan Krause on 2026-06-08.
//

#include "components/hertziandipole.hpp"

HertzianDipole::HertzianDipole(std::string_view const id, Reference const &origin, double const length) : Radiator(id, origin), length(length) {}

Vec3 HertzianDipole::calc_polar_effective_length(double const theta, double phi) const { return {0, -length * std::sin(theta), 0}; }

complex_t HertzianDipole::calc_radiation_gain(Vec3 const &pos, double freq) const
{
    throw std::runtime_error("not implemented");
}
