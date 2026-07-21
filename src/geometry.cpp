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

        std::tuple<pos_t, pos_t, pos_t> normalize_base(Geometry geo)
        {
            auto const normal = get_normal(geo).normalize();
            auto const e1 = get_e1(geo).normalize();
            auto const e2 = normal.cross(e1);
            assert_orthogonality(normal, e1, e2, "circle");
            return {normal, e1, e2};
        }
    } // namespace

    CircleArc CircleArc::normalized() const
    {
        auto const [new_normal, new_e1, new_e2] = normalize_base(*this);
        return CircleArc{id, center, new_normal, new_e1, new_e2, radius, angle_span};
    }

    CircleArc CircleArc::rotate(double angle) const
    {
        auto const new_e1 = std::cos(angle) * e1 + std::sin(angle) * e2;
        return CircleArc{id, center, normal, new_e1, normal.cross(new_e1), radius, angle_span};
    }

    Rectangle Rectangle::normalized() const
    {
        auto const [new_normal, new_e1, new_e2] = normalize_base(*this);
        return Rectangle{id, center, new_normal, new_e1, new_e2, width, height};
    }

    SphericalRectangle SphericalRectangle::normalized() const
    {
        auto const [new_normal, new_e1, new_e2] = normalize_base(*this);
        return SphericalRectangle{id, center, new_normal, new_e1, new_e2, radius, polar_span, azimuth_span};
    }

    template <any_json_t JsonType>
    void to_json(JsonType& js, CircleArc const& c)
    { js = JsonType{{"center", c.center}, {"normal", c.normal}, {"e1", c.e1}, {"e2", c.e2}, {"radius", c.radius}, {"angle_span", c.angle_span}}; }

    template <any_json_t JsonType>
    void from_json(JsonType const& js, CircleArc& c)
    {
        serialization::assert_structure(js,
            "geometry::CircleArc",
            {
                {"center", json::value_t::object},
                {"normal", json::value_t::object},
                {"e1", json::value_t::object},
                {"radius", json::value_t::number_float},
                {"angle_span", json::value_t::number_float},
            },
            {{"e2", json::value_t::object}});
        c = CircleArc{js.at("id").template get<std::string>(),
            js.at("center").template get<pos_t>(),
            js.at("normal").template get<pos_t>(),
            js.at("e1").template get<pos_t>(),
            POS_ZERO,
            js.at("radius").template get<double>(),
            js.at("angle_span").template get<double>()}
                .normalized();
    }

    template <any_json_t JsonType>
    void to_json(JsonType& js, Rectangle const& r)
    { js = JsonType{{"center", r.center}, {"normal", r.normal}, {"e1", r.e1}, {"e2", r.e2}, {"width", r.width}, {"height", r.height}}; }

    template <any_json_t JsonType>
    void from_json(JsonType const& js, Rectangle& r)
    {
        serialization::assert_structure(js,
            "geometry::Rectangle",
            {
                {"center", json::value_t::object},
                {"normal", json::value_t::object},
                {"e1", json::value_t::object},
                {"width", json::value_t::number_float},
                {"height", json::value_t::number_float},
            },
            {{"e2", json::value_t::object}});
        r = Rectangle{js.at("id").template get<std::string>(),
            js.at("center").template get<pos_t>(),
            js.at("normal").template get<pos_t>(),
            js.at("e1").template get<pos_t>(),
            POS_ZERO,
            js.at("width").template get<double>(),
            js.at("height").template get<double>()}
                .normalized();
    }

    template <any_json_t JsonType>
    void to_json(JsonType& js, SphericalRectangle const& sr)
    {
        js = JsonType{{"center", sr.center},
            {"normal", sr.normal},
            {"e1", sr.e1},
            {"e2", sr.e2},
            {"radius", sr.radius},
            {"polar_span", sr.polar_span},
            {"azimuth_span", sr.azimuth_span}};
    }

    template <any_json_t JsonType>
    void from_json(JsonType const& js, SphericalRectangle& sr)
    {
        serialization::assert_structure(js,
            "geometry::SphericalRectangle",
            {
                {"center", json::value_t::object},
                {"normal", json::value_t::object},
                {"e1", json::value_t::object},
                {"radius", json::value_t::number_float},
                {"polar_span", json::value_t::number_float},
                {"azimuth_span", json::value_t::number_float},
            },
            {{"e2", json::value_t::object}});
        sr = SphericalRectangle{js.at("id").template get<std::string>(),
            js.at("center").template get<pos_t>(),
            js.at("normal").template get<pos_t>(),
            js.at("e1").template get<pos_t>(),
            POS_ZERO,
            js.at("radius").template get<double>(),
            js.at("polar_span").template get<double>(),
            js.at("azimuth_span").template get<double>()}.normalized();
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
