// in
//  Created by Tristan Krause on 2026-05-29.
//

#pragma once

#include "types.hpp"

struct Reference
{
    Reference(std::string_view id, Reference const *origin, Vec3 const &translation = {}, Quaternion const &rotation = {});

    Reference(Reference const &) = delete; // disable copy constructor
    Reference &operator=(Reference const &) = delete; // disable copy assignment
    Reference(Reference &&) = delete; // disable move constructor
    Reference &operator=(Reference &&) = delete; // disable move assignment

    [[nodiscard]] Vec3 local_from_global(Vec3 const &pos_global) const;
    [[nodiscard]] Vec3 global_from_local(Vec3 const &pos_local) const;
    [[nodiscard]] Vec3 localize(Reference const& reference) const;

    std::string const id;
    Reference const *origin;
    Vec3 pos;
    Quaternion rotation;
};
