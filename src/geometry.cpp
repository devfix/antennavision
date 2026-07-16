//
// Created by core on 2026-07-14.
//

#include "geometry.hpp"
#include <nlohmann/json.hpp>
#include "serialization.hpp"
#include "simulationerror.hpp"

namespace geometry
{
    namespace
    {
        void assert_orthogonality(pos_t const& normal, pos_t const& e1, pos_t const& e2, std::string_view object)
        {
            if (std::abs(normal.dot(e1)) > NUMERICAL_MARGIN) { throw SimulationError("Invalid {} definition: normal and e1 must be orthogonal", object); }
            if (std::abs(normal.dot(e2)) > NUMERICAL_MARGIN) { throw SimulationError("Invalid {} definition: normal and e2 must be orthogonal", object); }
            if (std::abs(e1.dot(e2)) > NUMERICAL_MARGIN) { throw SimulationError("Invalid {} definition: e1 and e2 must be orthogonal", object); }
        }
    } // namespace

    CircleArc::CircleArc(pos_t const& center, pos_t const& normal, pos_t const& e1, double radius, double angle_span) :
        center_(center), normal_(normal.normalize()), e1_(e1.normalize()), e2_(normal_.cross(e1_)), radius_(radius), angle_span_(angle_span)
    { assert_orthogonality(normal_, e1_, e2_, "circle"); }

    CircleArc CircleArc::rotate(double angle) const { return {center_, normal_, std::cos(angle) * e1_ + std::sin(angle) * e2_, radius_, angle_span_}; }

    template <any_json_t JsonType>
    void to_json(JsonType& j, CircleArc const& c)
    {
        j = JsonType{
            {"center", c.center()}, {"normal", c.normal()}, {"e1", c.e1()}, {"e2", c.e2()}, {"radius", c.radius()}, {"angle_span", c.angle_span()}};
    }

    template <any_json_t JsonType>
    void from_json(JsonType const& j, CircleArc& c)
    {
        serialization::assert_structure(j, "geometry::CircleArc",
            {
                {"center", json::value_t::object},
                {"normal", json::value_t::object},
                {"e1", json::value_t::object},
                {"radius", json::value_t::number_float},
                {"angle_span", json::value_t::number_float},
            },
            {{"e2", json::value_t::object}});
        pos_t center;
        pos_t normal;
        pos_t e1;
        double radius;
        double angle_span;
        j.at("center").get_to(center);
        j.at("normal").get_to(normal);
        j.at("e1").get_to(e1);
        j.at("radius").get_to(radius);
        j.at("angle_span").get_to(angle_span);
        c = CircleArc(center, normal, e1, radius, angle_span);
    }

    Rectangle::Rectangle(pos_t const& center, pos_t const& normal, pos_t const& e1, double width, double height) :
        center_(center), normal_(normal.normalize()), e1_(e1.normalize()), e2_(normal_.cross(e1_)), width_(width), height_(height)
    { assert_orthogonality(normal_, e1_, e2_, "rectangle"); }

    template <any_json_t JsonType>
    void to_json(JsonType& j, Rectangle const& r)
    { j = JsonType{{"center", r.center()}, {"normal", r.normal()}, {"e1", r.e1()}, {"e2", r.e2()}, {"width", r.width()}, {"height", r.height()}}; }

    template <any_json_t JsonType>
    void from_json(JsonType const& j, Rectangle& r)
    {
        serialization::assert_structure(j, "geometry::Rectangle",
            {
                {"center", json::value_t::object},
                {"normal", json::value_t::object},
                {"e1", json::value_t::object},
                {"width", json::value_t::number_float},
                {"height", json::value_t::number_float},
            },
            {{"e2", json::value_t::object}});
        pos_t center;
        pos_t normal;
        pos_t e1;
        double width;
        double height;
        j.at("center").get_to(center);
        j.at("normal").get_to(normal);
        j.at("e1").get_to(e1);
        j.at("width").get_to(width);
        j.at("height").get_to(height);
        r = Rectangle(center, normal, e1, width, height);
    }

    SphericalRectangle::SphericalRectangle(pos_t const& center, pos_t const& normal, pos_t const& e1, double radius, double polar_span, double azimuth_span) :
        center_(center), normal_(normal.normalize()), e1_(e1.normalize()), e2_(normal_.cross(e1_)), radius_(radius), polar_span_(polar_span),
        azimuth_span_(azimuth_span)
    { assert_orthogonality(normal_, e1_, e2_, "spherical rectangle"); }

    template <any_json_t JsonType>
    void to_json(JsonType& j, SphericalRectangle const& sr)
    {
        j = JsonType{{"center", sr.center()}, {"normal", sr.normal()}, {"e1", sr.e1()}, {"e2", sr.e2()}, {"radius", sr.radius()},
            {"polar_span", sr.polar_span()}, {"azimuth_span", sr.azimuth_span()}};
    }

    template <any_json_t JsonType>
    void from_json(JsonType const& j, SphericalRectangle& sr)
    {
        serialization::assert_structure(j, "geometry::SphericalRectangle",
            {
                {"center", json::value_t::object},
                {"normal", json::value_t::object},
                {"e1", json::value_t::object},
                {"radius", json::value_t::number_float},
                {"polar_span", json::value_t::number_float},
                {"azimuth_span", json::value_t::number_float},
            },
            {{"e2", json::value_t::object}});
        pos_t center;
        pos_t normal;
        pos_t e1;
        double radius;
        double polar_span;
        double azimuth_span;
        j.at("center").get_to(center);
        j.at("normal").get_to(normal);
        j.at("e1").get_to(e1);
        j.at("radius").get_to(radius);
        j.at("polar_span").get_to(polar_span);
        j.at("azimuth_span").get_to(azimuth_span);
        sr = SphericalRectangle(center, normal, e1, radius, polar_span, azimuth_span);
    }
} // namespace geometry

// Circle Instantiations
template void geometry::to_json(nlohmann::json&, geometry::CircleArc const&);
template void geometry::to_json(nlohmann::ordered_json&, geometry::CircleArc const&);
template void geometry::from_json(nlohmann::json const&, geometry::CircleArc&);
template void geometry::from_json(nlohmann::ordered_json const&, geometry::CircleArc&);

// Rectangle Instantiations
template void geometry::to_json(nlohmann::json&, geometry::Rectangle const&);
template void geometry::to_json(nlohmann::ordered_json&, geometry::Rectangle const&);
template void geometry::from_json(nlohmann::json const&, geometry::Rectangle&);
template void geometry::from_json(nlohmann::ordered_json const&, geometry::Rectangle&);

// SphericalRectangle Instantiations
template void geometry::to_json(nlohmann::json&, geometry::SphericalRectangle const&);
template void geometry::to_json(nlohmann::ordered_json&, geometry::SphericalRectangle const&);
template void geometry::from_json(nlohmann::json const&, geometry::SphericalRectangle&);
template void geometry::from_json(nlohmann::ordered_json const&, geometry::SphericalRectangle&);
