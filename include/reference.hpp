// in
//  Created by Tristan Krause on 2026-05-29.
//

#pragma once

#include "types.hpp"

struct Reference
{
    Reference(std::string_view id, Reference const* origin, Vec3 const& translation = {}, Quaternion const& rotation = {});

    Reference(Reference const&) = delete; // disable copy constructor
    Reference& operator=(Reference const&) = delete; // disable copy assignment
    Reference(Reference&&) = delete; // disable move constructor
    Reference& operator=(Reference&&) = delete; // disable move assignment

    // [[nodiscard]] Reference copy() const;

    [[nodiscard]] pos_t local_from_global_pos(pos_t const& pos_global) const;
    [[nodiscard]] pos_t global_from_local_pos(pos_t const& pos_local) const;
    [[nodiscard]] vec_t local_from_global_vec(vec_t const& vec_global) const;
    [[nodiscard]] vec_t global_from_local_vec(vec_t const& vec_local) const;
    [[nodiscard]] pos_t localize(Reference const& reference) const;

    std::string const id;
    Reference const* origin;
    Vec3 pos;
    Quaternion rotation;
};
