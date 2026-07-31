//
// Created by Tristan Krause on 2026-07-14.
//

#pragma once

#include <utility>
#include <variant>
#include "serialization.hpp"
#include "types/json.hpp"
#include "types/math.hpp"
#include "math/functions.hpp"

namespace geometry
{
    struct Line
    {
        static constexpr std::string_view name = "Line";

        [[nodiscard]] Line() = default;

        [[nodiscard]] Line(std::string id, Pos const& pos1, Pos const& pos2) : id_(std::move(id)), pos_begin_(pos1), pos_end_(pos2) {}

        [[nodiscard]] std::string const& id() const { return id_; }

        [[nodiscard]] Pos const& pos_begin() const { return pos_begin_; }

        [[nodiscard]] Pos const& pos_end() const { return pos_end_; }

        [[nodiscard]] double length() const { return (pos_end_ - pos_begin_).norm(); }

        [[nodiscard]] Pos constexpr pos_at(double t) const { return math::lerp(pos_begin_, pos_end_, t); }

    private:
        std::string id_; /// id of the geometry
        Pos pos_begin_; /// first position of the line (start)
        Pos pos_end_; /// second position of the line (end)
    };

    //             ↑ e2 axis (angle=90°)
    //             |
    //         . . | . .
    //       .     |     .             ⊙ normal (up in the circle's plane)
    //     .       |       .
    //    .        |        .
    //   .         |        X
    //   .         |         X
    // --.---------C---------X---------→ e1 axis (angle=0°)
    //   .       (Center)    X
    //    .        |        X 🡤
    //     .       |       .   the arc
    //       .     |     .
    //         . . | . .
    //             |
    struct CircleArc
    {
        static constexpr std::string_view name = "CircleArc";

        [[nodiscard]] CircleArc() = default;

        [[nodiscard]] CircleArc( //
            std::string id,
            Pos const& center,
            Pos const& normal,
            Pos const& e1,
            Pos const& e2,
            double radius,
            double angle_span) //
            : id_(std::move(id)), center_(center), normal_(normal), e1_(e1), e2_(e2), radius_(radius), angle_span_(angle_span)
        {}

        [[nodiscard]] std::string const& id() const { return id_; }

        [[nodiscard]] Pos const& center() const { return center_; }

        [[nodiscard]] Pos const& normal() const { return normal_; }

        [[nodiscard]] Pos const& e1() const { return e1_; }

        [[nodiscard]] Pos const& e2() const { return e2_; }

        [[nodiscard]] double radius() const { return radius_; }

        [[nodiscard]] double angle_span() const { return angle_span_; }

        [[nodiscard]] double length() const { return radius_ * angle_span_; }

        [[nodiscard]] Pos constexpr pos_at_angle(double angle) const noexcept { return center_ + radius_ * (std::cos(angle) * e1_ + std::sin(angle) * e2_); }

        [[nodiscard]] double constexpr angle_at(double t) const noexcept { return (t - 0.5) * angle_span_; }

        [[nodiscard]] Pos constexpr pos_at(double t) const noexcept { return pos_at_angle(angle_at(t)); }

        [[nodiscard]] CircleArc normalized() const;
        [[nodiscard]] CircleArc rotate(double angle) const;

    private:
        std::string id_; /// id of the geometry
        Pos center_; /// center position
        Pos normal_; /// normal direction, together with center defines circle plane
        Pos e1_; /// first unit vector (see ascii sketch)
        Pos e2_; /// second unit vector (see ascii sketch)
        double radius_{}; /// circle radius
        double angle_span_{}; /// angle span of the arc
    };

    //  <------ width ------>
    //  +-------------------+   ^
    //  |                   |   |
    //  |  vec e2 ↑         |   |      ⊙ normal (up in the rectangle's plane)
    //  |         |         |   |
    //  |  center *--→      | height
    //  |            vec e1 |   |
    //  |                   |   |
    //  |                   |   |
    //  +-------------------+   v
    struct Rectangle
    {
        static constexpr std::string_view name = "Rectangle";

        [[nodiscard]] Rectangle() = default;

        [[nodiscard]] Rectangle( //
            std::string id,
            Pos const& center,
            Pos const& normal,
            Pos const& e1,
            Pos const& e2,
            double width,
            double height) //
            : id_(std::move(id)), center_(center), normal_(normal), e1_(e1), e2_(e2), width_(width), height_(height)
        {}

        [[nodiscard]] std::string const& id() const { return id_; }

        [[nodiscard]] Pos const& center() const { return center_; }

        [[nodiscard]] Pos const& normal() const { return normal_; }

        [[nodiscard]] Pos const& e1() const { return e1_; }

        [[nodiscard]] Pos const& e2() const { return e2_; }

        [[nodiscard]] double width() const { return width_; }

        [[nodiscard]] double height() const { return height_; }

        [[nodiscard]] Rectangle normalized() const;

        [[nodiscard]] double area() const { return width_ * height_; }

        [[nodiscard]] Pos constexpr pos_at(double t1, double t2) const { return center_ + (t1 - 0.5) * e1_ * width_ + (t2 - 0.5) * e2_ * height_; }

    private:
        std::string id_; /// id of the geometry
        Pos center_; /// center position
        Pos normal_; /// normal direction, together with center defines rectangle plane
        Pos e1_; /// first unit vector (see ascii sketch)
        Pos e2_; /// second unit vector (see ascii sketch)
        double width_{}; /// rectangle width, view from above if normal is pointing up
        double height_{}; /// rectangle height, view from above if normal is pointing up
    };

    struct SphericalRectangle
    {
        static constexpr std::string_view name = "SphericalRectangle";

        [[nodiscard]] SphericalRectangle() = default;

        [[nodiscard]] SphericalRectangle( //
            std::string id,
            Pos const& center,
            Pos const& normal,
            Pos const& e1,
            Pos const& e2,
            double radius,
            double polar_span,
            double azimuth_span) //
            : id_(std::move(id)), center_(center), normal_(normal), e1_(e1), e2_(e2), radius_(radius), polar_span_(polar_span), azimuth_span_(azimuth_span)
        {}

        [[nodiscard]] std::string const& id() const { return id_; }

        [[nodiscard]] Pos const& center() const { return center_; }

        [[nodiscard]] Pos const& normal() const { return normal_; }

        [[nodiscard]] Pos const& e1() const { return e1_; }

        [[nodiscard]] Pos const& e2() const { return e2_; }

        [[nodiscard]] double radius() const { return radius_; }

        [[nodiscard]] double polar_span() const { return polar_span_; }

        [[nodiscard]] double azimuth_span() const { return azimuth_span_; }

        [[nodiscard]] SphericalRectangle normalized() const;

        [[nodiscard]] double area() const;

        [[nodiscard]] Pos pos_at(double t1, double t2) const;

    private:
        std::string id_; /// id of the geometry
        Pos center_; /// center position of the sphere
        Pos normal_; /// surface normal at the center of the curved surface
        Pos e1_; /// first tangent unit vector
        Pos e2_; /// second tangent unit vector
        double radius_{}; /// sphere radius
        double polar_span_{}; /// total span of polar angle
        double azimuth_span_{}; /// total span of azimuthal angle
    };

    using Geometry = std::variant<Line, CircleArc, Rectangle, SphericalRectangle>;

    template <AnyJson JsonType>
    void to_json(JsonType& js, Geometry const& geo);

    template <AnyJson JsonType>
    void from_json(JsonType const& js, Geometry& geo);

    [[nodiscard]] constexpr std::string const& get_id(Geometry const& geo) noexcept
    {
        return std::visit([](auto const& g) -> std::string const& { return g.id(); }, geo);
    }

    using Curve = std::variant<Line, CircleArc>;

    template <AnyJson JsonType>
    void to_json(JsonType& js, Curve const& curve)
    {
        std::visit([&js](auto const& c) { js = c; }, curve);
    }

    using Surface = std::variant<Rectangle, SphericalRectangle>;

    template <AnyJson JsonType>
    void to_json(JsonType& js, Surface const& surf)
    {
        std::visit([&js](auto const& s) { js = s; }, surf);
    }

    [[nodiscard]] Geometry& get(std::span<Geometry> geometries, std::string const& id);

    Vec3Array get_positions(Geometry const& geo, std::size_t n_dim1, std::size_t n_dim2);

    namespace curve
    {
        [[nodiscard]] constexpr double get_length(Curve const& curve) noexcept
        {
            return std::visit([](auto const& c) -> double { return c.length(); }, curve);
        }

        [[nodiscard]] constexpr Pos get_pos_at(Curve const& curve, double t) noexcept
        {
            return std::visit([&t](auto const& c) -> Pos { return c.pos_at(t); }, curve);
        }
    } // namespace curve

    namespace surface
    {
        [[nodiscard]] constexpr Pos const& get_center(Surface const& surf) noexcept
        {
            return std::visit([](auto const& gt) -> Pos const& { return gt.center(); }, surf);
        }

        [[nodiscard]] constexpr Pos const& get_normal(Surface const& surf) noexcept
        {
            return std::visit([](auto const& gt) -> Pos const& { return gt.normal(); }, surf);
        }

        [[nodiscard]] constexpr Pos const& get_e1(Surface const& surf) noexcept
        {
            return std::visit([](auto const& gt) -> Pos const& { return gt.e1(); }, surf);
        }

        [[nodiscard]] constexpr Pos const& get_e2(Surface const& surf) noexcept
        {
            return std::visit([](auto const& gt) -> Pos const& { return gt.e2(); }, surf);
        }

        [[nodiscard]] constexpr double get_area(Surface const& surf) noexcept
        {
            return std::visit([](auto const& s) -> double { return s.area(); }, surf);
        }

        [[nodiscard]] constexpr Pos get_pos_at(Surface const& surf, double t1, double t2) noexcept
        {
            return std::visit([&t1, &t2](auto const& s) -> Pos { return s.pos_at(t1, t2); }, surf);
        }
    } // namespace surface

} // namespace geometry
