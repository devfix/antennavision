//
// Created by Tristan Krause on 2026-05-29.
//

#include "reference.hpp"

Reference::Reference(std::string_view const id, Reference const* origin, Vec3 const &translation, Quaternion const &rotation) :
   id(id), origin(origin), pos(translation), rotation(rotation)
{
}

Vec3 Reference::local_from_global(Vec3 const &pos_global) const
{
    return rotation.inverse().rotate((origin ? origin->local_from_global(pos_global) : pos_global) - this->pos);
}

Vec3 Reference::global_from_local(Vec3 const &pos_local) const
{
    Vec3 const pos_global = rotation.rotate(pos_local) + this->pos;
    return origin ? origin->global_from_local(pos_global) : pos_global;
}

Vec3 Reference::localize(Reference const &reference) const
{
    return local_from_global(reference.global_from_local(POS_ZERO));
}
