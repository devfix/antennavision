//
// Created by core on 2026-07-14.
//

#pragma once

#include <variant>

#include "types.hpp"

namespace geometry
{
    //             ^ e2 axis (90°)
    //             |
    //         . . | . .
    //       .     |     .             ⊙ normal (up in the circle's plane)
    //     .       |       .
    //    .        |        .
    //   .         |         .
    //   .         |         .
    // --.---------C---------.---------> e1 axis (0°, start_direction)
    //   .       (Center)    .
    //    .        |        .
    //     .       |       .
    //       .     |     .
    //         . . | . .
    //             |
    struct CircleArc
    {
        [[nodiscard]] CircleArc normalized() const;
        [[nodiscard]] CircleArc rotate(double angle) const;

        [[nodiscard]] pos_t constexpr get_pos(double angle) const noexcept { return center + radius * (std::cos(angle) * e1 + std::sin(angle) * e2); }

        std::string id; /// id of the geometry
        pos_t center; /// center position
        pos_t normal; /// normal direction, together with center defines circle plane
        pos_t e1; /// first unit vector (see ascii sketch)
        pos_t e2; /// second unit vector (see ascii sketch)
        double radius{}; /// circle radius
        double angle_span{}; /// angle span of the arc
    };

    template <any_json_t JsonType>
    void to_json(JsonType& js, CircleArc const& c);

    template <any_json_t JsonType>
    void from_json(JsonType const& js, CircleArc& c);

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
        [[nodiscard]] Rectangle normalized() const;

        std::string id; /// id of the geometry
        pos_t center; /// center position
        pos_t normal; /// normal direction, together with center defines rectangle plane
        pos_t e1; /// first unit vector (see ascii sketch)
        pos_t e2; /// second unit vector (see ascii sketch)
        double width{}; /// rectangle width, view from above if normal is pointing up
        double height{}; /// rectangle height, view from above if normal is pointing up
    };

    template <any_json_t JsonType>
    void to_json(JsonType& js, Rectangle const& r);

    template <any_json_t JsonType>
    void from_json(JsonType const& js, Rectangle& r);

    struct SphericalRectangle
    {
        [[nodiscard]] SphericalRectangle normalized() const;

        std::string id; /// id of the geometry
        pos_t center; /// center position of the sphere
        pos_t normal; /// surface normal at the center of the curved surface
        pos_t e1; /// first tangent unit vector
        pos_t e2; /// second tangent unit vector
        double radius{}; /// sphere radius
        double polar_span{}; /// total span of polar angle
        double azimuth_span{}; /// total span of azimuthal angle
    };

    template <any_json_t JsonType>
    void to_json(JsonType& js, SphericalRectangle const& sr);

    template <any_json_t JsonType>
    void from_json(JsonType const& js, SphericalRectangle& sr);
} // namespace geometry

using Geometry = std::variant<geometry::CircleArc, geometry::Rectangle, geometry::SphericalRectangle>;

namespace geometry
{
    [[nodiscard]] constexpr pos_t const& get_center(Geometry const& g) noexcept
    {
        return std::visit([](auto const& gt) -> pos_t const& { return gt.center; }, g);
    }

    [[nodiscard]] constexpr pos_t const& get_normal(Geometry const& g) noexcept
    {
        return std::visit([](auto const& gt) -> pos_t const& { return gt.normal; }, g);
    }

    [[nodiscard]] constexpr pos_t const& get_e1(Geometry const& g) noexcept
    {
        return std::visit([](auto const& gt) -> pos_t const& { return gt.e1; }, g);
    }

    [[nodiscard]] constexpr pos_t const& get_e2(Geometry const& g) noexcept
    {
        return std::visit([](auto const& gt) -> pos_t const& { return gt.e2; }, g);
    }

} // namespace geometry
