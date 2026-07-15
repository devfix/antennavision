//
// Created by core on 2026-07-14.
//

#include "geometry.hpp"
#include <nlohmann/json.hpp>
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

    template <typename BasicJsonType>
    void to_json(BasicJsonType& j, CircleArc const& c)
    {
        j = BasicJsonType{{"center", c.center()}, {"normal", c.normal()}, {"e1", c.e1()},
                          {"e2", c.e2()},         {"radius", c.radius()}, {"angle_span", c.angle_span()}};
    }

    template <typename BasicJsonType>
    void from_json(BasicJsonType const& j, CircleArc& c)
    {
        if (!j.contains("center")) { throw SimulationError("Invalid circle definition: missing center"); }
        if (!j.contains("normal")) { throw SimulationError("Invalid circle definition: missing normal"); }
        if (!j.contains("e1")) { throw SimulationError("Invalid circle definition: missing e1"); }
        if (!j.contains("radius")) { throw SimulationError("Invalid circle definition: missing radius"); }
        if (!j.contains("angle_span")) { throw SimulationError("Invalid circle definition: missing angle_span"); }
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

    template <typename BasicJsonType>
    void to_json(BasicJsonType& j, Rectangle const& r)
    { j = BasicJsonType{{"center", r.center()}, {"normal", r.normal()}, {"e1", r.e1()}, {"e2", r.e2()}, {"width", r.width()}, {"height", r.height()}}; }

    template <typename BasicJsonType>
    void from_json(BasicJsonType const& j, Rectangle& r)
    {
        if (!j.contains("center")) { throw SimulationError("Invalid rectangle definition: missing center"); }
        if (!j.contains("normal")) { throw SimulationError("Invalid rectangle definition: missing normal"); }
        if (!j.contains("e1")) { throw SimulationError("Invalid rectangle definition: missing e1"); }
        if (!j.contains("width")) { throw SimulationError("Invalid rectangle definition: missing width"); }
        if (!j.contains("height")) { throw SimulationError("Invalid rectangle definition: missing height"); }
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

    template <typename BasicJsonType>
    void to_json(BasicJsonType& j, SphericalRectangle const& sr)
    {
        j = BasicJsonType{{"center", sr.center()},
                          {"normal", sr.normal()},
                          {"e1", sr.e1()},
                          {"e2", sr.e2()},
                          {"radius", sr.radius()},
                          {"polar_span", sr.polar_span()},
                          {"azimuth_span", sr.azimuth_span()}};
    }

    template <typename BasicJsonType>
    void from_json(BasicJsonType const& j, SphericalRectangle& sr)
    {
        if (!j.contains("center")) { throw SimulationError("Invalid spherical rectangle definition: missing center"); }
        if (!j.contains("normal")) { throw SimulationError("Invalid spherical rectangle definition: missing normal"); }
        if (!j.contains("e1")) { throw SimulationError("Invalid spherical rectangle definition: missing e1"); }
        if (!j.contains("radius")) { throw SimulationError("Invalid spherical rectangle definition: missing radius"); }
        if (!j.contains("polar_span")) { throw SimulationError("Invalid spherical rectangle definition: missing polar_span"); }
        if (!j.contains("azimuth_span")) { throw SimulationError("Invalid spherical rectangle definition: missing azimuth_span"); }
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
