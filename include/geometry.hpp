//
// Created by core on 2026-07-14.
//

#pragma once

#include <variant>

#include "types/json.hpp"
#include "types/math.hpp"

namespace geometry
{
    struct Line
    {
        [[nodiscard]] Line(std::string const& id, pos_t const& pos1, pos_t const& pos2) : id_(id), pos1_(pos1), pos2_(pos2) {}

        [[nodiscard]] std::string id() const { return id_; }

        [[nodiscard]] pos_t pos1() const { return pos1_; }

        [[nodiscard]] pos_t pos2() const { return pos2_; }

        [[nodiscard]] double length() const { return (pos2_ - pos1_).norm(); }

        [[nodiscard]] pos_t constexpr pos_at(double t) const { return pos1_ + t * (pos2_ - pos1_); }

    private:
        std::string id_; /// id of the geometry
        pos_t pos1_; /// first position of the line (start)
        pos_t pos2_; /// second position of the line (end)
    };

    template <any_json_t JsonType>
    void to_json(JsonType& js, Line const& l);

    template <any_json_t JsonType>
    void from_json(JsonType const& js, Line& l);

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
        [[nodiscard]] CircleArc( //
            std::string const& id,
            pos_t const& center,
            pos_t const& normal,
            pos_t const& e1,
            pos_t const& e2,
            double radius,
            double angle_span) //
            : id_(id), center_(center), normal_(normal), e1_(e1), e2(e2), radius_(radius), angle_span_(angle_span)
        {}

        [[nodiscard]] std::string id() const { return id_; }

        [[nodiscard]] pos_t center() const { return center_; }

        [[nodiscard]] pos_t normal() const { return normal_; }

        [[nodiscard]] pos_t e1() const { return e1_; }

        [[nodiscard]] pos_t e3() const { return e2; }

        [[nodiscard]] double radius() const { return radius_; }

        [[nodiscard]] double angle_span() const { return angle_span_; }

        [[nodiscard]] double length() const { return radius_ * angle_span_; }

        [[nodiscard]] pos_t constexpr pos_at_angle(double angle) const noexcept { return center_ + radius_ * (std::cos(angle) * e1_ + std::sin(angle) * e2); }

        [[nodiscard]] pos_t constexpr pos_at(double t) const noexcept { return pos_at_angle((t - 0.5) * angle_span_); }

        [[nodiscard]] CircleArc normalized() const;
        [[nodiscard]] CircleArc rotate(double angle) const;

    private:
        std::string id_; /// id of the geometry
        pos_t center_; /// center position
        pos_t normal_; /// normal direction, together with center defines circle plane
        pos_t e1_; /// first unit vector (see ascii sketch)
        pos_t e2; /// second unit vector (see ascii sketch)
        double radius_{}; /// circle radius
        double angle_span_{}; /// angle span of the arc
        CircleArc() = default;
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
        [[nodiscard]] Rectangle(//
            std::string const& id,
            pos_t const& center,
            pos_t const& normal,
            pos_t const& e1,
            pos_t const& e2,
            double width,
            double height) //
            : id_(id), center_(center), normal_(normal), e1_(e1), e2_(e2), width_(width), height_(height)
        {}

        [[nodiscard]] std::string id() const { return id_; }

        [[nodiscard]] pos_t center() const { return center_; }

        [[nodiscard]] pos_t normal() const { return normal_; }

        [[nodiscard]] pos_t e1() const { return e1_; }

        [[nodiscard]] pos_t e2() const { return e2_; }

        [[nodiscard]] double width() const { return width_; }

        [[nodiscard]] double height() const { return height_; }

        [[nodiscard]] Rectangle normalized() const;

        [[nodiscard]] pos_t constexpr pos_at(double t1, double t2) const { return center_ + (t1 - 0.5) * e1_ * width_ + (t2 - 0.5) + e2_ * height_; }

    private:
        std::string id_; /// id of the geometry
        pos_t center_; /// center position
        pos_t normal_; /// normal direction, together with center defines rectangle plane
        pos_t e1_; /// first unit vector (see ascii sketch)
        pos_t e2_; /// second unit vector (see ascii sketch)
        double width_{}; /// rectangle width, view from above if normal is pointing up
        double height_{}; /// rectangle height, view from above if normal is pointing up
    };

    template <any_json_t JsonType>
    void to_json(JsonType& js, Rectangle const& r);

    template <any_json_t JsonType>
    void from_json(JsonType const& js, Rectangle& r);

    struct SphericalRectangle
    {
        [[nodiscard]] SphericalRectangle(//
            std::string const& id,
            pos_t const& center,
            pos_t const& normal,
            pos_t const& e1,
            pos_t const& e2,
            double radius,
            double polar_span,
            double azimuth_span) //
            : id_(id), center_(center), normal_(normal), e1_(e1), e2_(e2), radius_(radius), polar_span_(polar_span), azimuth_span_(azimuth_span)
        {}

        [[nodiscard]] std::string id() const { return id_; }

        [[nodiscard]] pos_t center() const { return center_; }

        [[nodiscard]] pos_t normal() const { return normal_; }

        [[nodiscard]] pos_t e1() const { return e1_; }

        [[nodiscard]] pos_t e2() const { return e2_; }

        [[nodiscard]] double radius() const { return radius_; }

        [[nodiscard]] double polar_span() const { return polar_span_; }

        [[nodiscard]] double azimuth_span() const { return azimuth_span_; }

        [[nodiscard]] SphericalRectangle normalized() const;

        [[nodiscard]] pos_t pos_at(double t1, double t2) const;

    private:
        std::string id_; /// id of the geometry
        pos_t center_; /// center position of the sphere
        pos_t normal_; /// surface normal at the center of the curved surface
        pos_t e1_; /// first tangent unit vector
        pos_t e2_; /// second tangent unit vector
        double radius_{}; /// sphere radius
        double polar_span_{}; /// total span of polar angle
        double azimuth_span_{}; /// total span of azimuthal angle
    };

    template <any_json_t JsonType>
    void to_json(JsonType& js, SphericalRectangle const& sr);

    template <any_json_t JsonType>
    void from_json(JsonType const& js, SphericalRectangle& sr);

    using Curve = std::variant<Line, CircleArc>;
    using Surface = std::variant<Rectangle, SphericalRectangle>;
    using Geometry = std::variant<Line, CircleArc, Rectangle, SphericalRectangle>;

    [[nodiscard]] constexpr std::string const& get_id(Geometry const& geo) noexcept
    {
        return std::visit([](auto const& gt) -> std::string const& { return gt.id(); }, geo);
    }

    [[nodiscard]] Geometry& get(std::span<Geometry> geometries, std::string const& id);

    namespace curve
    {
        [[nodiscard]] constexpr double get_length(Curve const& curve) noexcept
        {
            return std::visit([](auto const& c) -> double { return c.length(); }, curve);
        }

        [[nodiscard]] constexpr pos_t get_pos_at(Curve const& curve, double t) noexcept
        {
            return std::visit([&t](auto const& c) -> pos_t { return c.pos_at(t); }, curve);
        }
    } // namespace curve

    namespace surface
    {
        [[nodiscard]] constexpr pos_t const& get_center(Surface const& surf) noexcept
        {
            return std::visit([](auto const& gt) -> pos_t const& { return gt.center(); }, surf);
        }

        [[nodiscard]] constexpr pos_t const& get_normal(Surface const& surf) noexcept
        {
            return std::visit([](auto const& gt) -> pos_t const& { return gt.normal(); }, surf);
        }

        [[nodiscard]] constexpr pos_t const& get_e1(Surface const& surf) noexcept
        {
            return std::visit([](auto const& gt) -> pos_t const& { return gt.e1(); }, surf);
        }

        [[nodiscard]] constexpr pos_t const& get_e2(Surface const& surf) noexcept
        {
            return std::visit([](auto const& gt) -> pos_t const& { return gt.e2(); }, surf);
        }

        [[nodiscard]] constexpr pos_t get_pos_at(Surface const& surf, double t1, double t2) noexcept
        {
            return std::visit([&t1, &t2](auto const& s) -> pos_t { return s.pos_at(t1, t2); }, surf);
        }
    } // namespace surface

} // namespace geometry
