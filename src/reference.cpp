//
// Created by Tristan Krause on 2026-05-29.
//

#include "reference.hpp"

Reference::Reference(std::string_view const id, Reference const* origin, Vec3 const &translation, Quaternion const &rotation) :
   id(id), origin(origin), pos(translation), orientation(rotation), orientation_inverse(rotation.inverse())
{
}

Vec3 Reference::local_from_global(Vec3 const &pos) const
{
    return orientation_inverse.rotate((origin ? origin->local_from_global(pos) : pos) - this->pos);
}

Vec3 Reference::global_from_local(Vec3 const &pos) const
{
    Vec3 const pos_global = orientation.rotate(pos) + this->pos;
    return origin ? origin->global_from_local(pos_global) : pos_global;
}

Vec3 Reference::localize(Reference const &reference) const
{
    return local_from_global(reference.global_from_local(POS_ZERO));
}
