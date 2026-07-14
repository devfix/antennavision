//
// Created by core on 2026-07-14.
//

#pragma once

#include "types.hpp"


namespace geometry
{
    //             ^ v2 axis (90°)
    //             |
    //         . . | . .
    //       .     |     .             ⊙ normal (up in the circle's plane)
    //     .       |       .
    //    .        |        .
    //   .         |         .
    //   .         |         .
    // --.---------C---------.---------> v1 axis (0°, start_direction)
    //   .       (Center)    .
    //    .        |        .
    //     .       |       .
    //       .     |     .
    //         . . | . .
    //             |
    struct Circle
    {
        pos_t center; /// center position
        pos_t normal; /// normal direction, together with center defines circle plane
        double radius{}; /// circle radius
        pos_t e1; /// first unit vector (see ascii sketch)
        pos_t e2; /// second unit vector (see ascii sketch)

        static Circle make(pos_t const& center, pos_t const& normal, double radius, pos_t const& dir_start);
        [[nodiscard]] Circle rotate_base(double angle) const;
    };

    template <typename BasicJsonType>
    void to_json(BasicJsonType& j, Circle const& sr);

    template <typename BasicJsonType>
    void from_json(BasicJsonType const& j, Circle& sr);

    //         <---- width ---->
    //         +---------------+   ^
    //         |               |   |
    //         |               |   |      ⊙ normal (up in the rectangle's plane)
    //         |               |   |
    //         |       *       | height
    //         ^     center    |   |
    //         I               |   |
    //  vec e2 I               |   |
    //         O==> -----------+   v
    //        vec e1
    struct Rectangle
    {
        pos_t center; /// center position
        pos_t normal; /// normal direction, together with center defines rectangle plane
        double width{}; /// rectangle width, view from above if normal is pointing up
        double height{}; /// rectangle height, view from above if normal is pointing up
        pos_t e1; /// first unit vector (see ascii sketch)
        pos_t e2; /// second unit vector (see ascii sketch)
        static Rectangle make(pos_t const& pos_zero, pos_t const& pos_width_max, pos_t const& pos_height_max);
    };

    template <typename BasicJsonType>
    void to_json(BasicJsonType& j, Rectangle const& sr);

    template <typename BasicJsonType>
    void from_json(BasicJsonType const& j, Rectangle& sr);

    struct SphericalRectangle
    {
        pos_t center; /// center position of the sphere
        pos_t normal; /// surface normal at the center of the curved surface
        double radius{}; /// sphere radius
        double polar{}; /// total span of polar angle
        double azimuth{}; /// total span of azimuthal angle
        pos_t e1; /// first tangent unit vector
        pos_t e2; /// second tangent unit vector
        static SphericalRectangle make(pos_t const& center, pos_t const& pos_rect, double polar, double azimuth, pos_t const& dir_north);
    };

    template <typename BasicJsonType>
    void to_json(BasicJsonType& j, SphericalRectangle const& sr);

    template <typename BasicJsonType>
    void from_json(BasicJsonType const& j, SphericalRectangle& sr);


} // namespace geometry
