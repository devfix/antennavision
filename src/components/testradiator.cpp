//
// Created by Tristan Krause on 2026-04-28.
//

#include "components/testradiator.hpp"


TestRadiator::TestRadiator(std::string_view const id, Reference const& origin) :
    Radiator(id, origin)
{}

complex_t TestRadiator::calc_radiation_gain(Vec3 const& pos, double freq) const
{
    auto const rel_pos = origin.local_from_global(pos);
    auto distance = nc::norm(rel_pos.toNdArray())[0];
    return {distance,rel_pos.z};
}

// complex_t TestRadiator::get_relative_gain(position_t rel_pos, double freq) const
// {
//     double phi = std::arg(rel_pos);
//     if (phi > std::numbers::pi)
//     {
//         phi -= 2 * std::numbers::pi;
//     }
//     phi += 1e-16;
//     double r = std::abs(rel_pos) + 1e-16;
//     return {std::min(1.0 / (r * r), 10.0), std::min(1.0 / (phi * phi), 10.0)};
// }
