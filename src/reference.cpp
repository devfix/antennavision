//
// Created by Tristan Krause on 2026-05-29.
//

#include "reference.hpp"

#include "math.hpp"

Reference::Reference(std::string_view const id, Reference const *origin, Vec3 const &translation, Quaternion const &rotation) : id(id), origin(origin), pos(translation), rotation(rotation) {}

pos_t Reference::local_from_global_pos(pos_t const &pos_global) const { return rotation.inverse().rotate((origin ? origin->local_from_global_pos(pos_global) : pos_global) - this->pos); }

pos_t Reference::global_from_local_pos(pos_t const &pos_local) const
{
    pos_t const pos_global = rotation.rotate(pos_local) + this->pos;
    return origin ? origin->global_from_local_pos(pos_global) : pos_global;
}

vec_t Reference::local_from_global_vec(vec_t const &vec_global) const
{
    return math::rotate(origin ? origin->local_from_global_vec(vec_global) : vec_global, rotation.inverse());
}

vec_t Reference::global_from_local_vec(vec_t const &vec_local) const
{
    vec_t const vec_parent = math::rotate(vec_local, rotation);
    return origin ? origin->global_from_local_vec(vec_parent) : vec_parent;
}

pos_t Reference::localize(Reference const &reference) const { return local_from_global_pos(reference.global_from_local_pos(POS_ZERO)); }
