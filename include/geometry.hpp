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
        CircleArc() = default;
        CircleArc( pos_t const& center, pos_t const& normal, pos_t const& e1, double radius, double angle_span);
        [[nodiscard]] CircleArc rotate(double angle) const;

    private:
        pos_t center_; /// center position
        pos_t normal_; /// normal direction, together with center defines circle plane
        pos_t e1_; /// first unit vector (see ascii sketch)
        pos_t e2_; /// second unit vector (see ascii sketch)
        double radius_{}; /// circle radius
        double angle_span_{}; /// angle span of the arc

    public:
        [[nodiscard]] constexpr pos_t const& center() const noexcept { return center_; }

        [[nodiscard]] constexpr pos_t const& normal() const noexcept { return normal_; }

        [[nodiscard]] constexpr pos_t const& e1() const noexcept { return e1_; }

        [[nodiscard]] constexpr pos_t const& e2() const noexcept { return e2_; }

        [[nodiscard]] constexpr double radius() const noexcept { return radius_; }

        [[nodiscard]] constexpr double angle_span() const noexcept { return angle_span_; }

        [[nodiscard]] constexpr pos_t get_pos(double angle) const noexcept { return center_ + radius_ * (std::cos(angle) * e1_ + std::sin(angle) * e2_); }
    };

    template <any_json_t JsonType>
    void to_json(JsonType& j, CircleArc const& c);

    template <any_json_t JsonType>
    void from_json(JsonType const& j, CircleArc& c);

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
        Rectangle() = default;
        Rectangle( pos_t const& center, pos_t const& normal, pos_t const& e1, double width, double height);

    private:
        pos_t center_; /// center position
        pos_t normal_; /// normal direction, together with center defines rectangle plane
        pos_t e1_; /// first unit vector (see ascii sketch)
        pos_t e2_; /// second unit vector (see ascii sketch)
        double width_{}; /// rectangle width, view from above if normal is pointing up
        double height_{}; /// rectangle height, view from above if normal is pointing up

    public:
        [[nodiscard]] constexpr pos_t const& center() const noexcept { return center_; }

        [[nodiscard]] constexpr pos_t const& normal() const noexcept { return normal_; }

        [[nodiscard]] constexpr pos_t const& e1() const noexcept { return e1_; }

        [[nodiscard]] constexpr pos_t const& e2() const noexcept { return e2_; }

        [[nodiscard]] constexpr double width() const noexcept { return width_; }

        [[nodiscard]] constexpr double height() const noexcept { return height_; }
    };

    template <any_json_t JsonType>
    void to_json(JsonType& j, Rectangle const& r);

    template <any_json_t JsonType>
    void from_json(JsonType const& j, Rectangle& r);

    struct SphericalRectangle
    {
        SphericalRectangle() = default;
        SphericalRectangle( pos_t const& center, pos_t const& normal, pos_t const& e1, double radius, double polar_span, double azimuth_span);

    private:
        std::string id_;
        pos_t center_; /// center position of the sphere
        pos_t normal_; /// surface normal at the center of the curved surface
        pos_t e1_; /// first tangent unit vector
        pos_t e2_; /// second tangent unit vector
        double radius_{}; /// sphere radius
        double polar_span_{}; /// total span of polar angle
        double azimuth_span_{}; /// total span of azimuthal angle

    public:
        [[nodiscard]] constexpr std::string const& id() const noexcept { return id_; }

        [[nodiscard]] constexpr pos_t const& center() const noexcept { return center_; }

        [[nodiscard]] constexpr pos_t const& normal() const noexcept { return normal_; }

        [[nodiscard]] constexpr pos_t const& e1() const noexcept { return e1_; }

        [[nodiscard]] constexpr pos_t const& e2() const noexcept { return e2_; }

        [[nodiscard]] constexpr double radius() const noexcept { return radius_; }

        [[nodiscard]] constexpr double polar_span() const noexcept { return polar_span_; }

        [[nodiscard]] constexpr double azimuth_span() const noexcept { return azimuth_span_; }
    };

    template <any_json_t JsonType>
    void to_json(JsonType& j, SphericalRectangle const& sr);

    template <any_json_t JsonType>
    void from_json(JsonType const& j, SphericalRectangle& sr);
} // namespace geometry

using Geometry = std::variant<geometry::CircleArc, geometry::Rectangle, geometry::SphericalRectangle>;

namespace geometry
{
    [[nodiscard]] constexpr pos_t const& get_center(Geometry const& g) noexcept
    {
        return std::visit([](auto const& gt) -> pos_t const& { return gt.center(); }, g);
    }

    [[nodiscard]] constexpr pos_t const& get_normal(Geometry const& g) noexcept
    {
        return std::visit([](auto const& gt) -> pos_t const& { return gt.normal(); }, g);
    }

    [[nodiscard]] constexpr pos_t const& get_e1(Geometry const& g) noexcept
    {
        return std::visit([](auto const& gt) -> pos_t const& { return gt.e1(); }, g);
    }

    [[nodiscard]] constexpr pos_t const& get_e2(Geometry const& g) noexcept
    {
        return std::visit([](auto const& gt) -> pos_t const& { return gt.e2(); }, g);
    }

} // namespace geometry
