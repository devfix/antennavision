//
// Created by core on 2026-07-14.
//

#include "geometry.hpp"
#include <nlohmann/json.hpp>

#include "math.hpp"
#include "memory.hpp"
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

        std::tuple<pos_t, pos_t, pos_t> normalize_base(pos_t const& e1, pos_t const& e2, pos_t const& normal, std::string_view object)
        {
            auto const new_normal = normal.normalize();
            auto const new_e1 = e1.normalize();
            auto const new_e2 = new_normal.cross(new_e1);
            assert_orthogonality(new_normal, new_e1, new_e2, object);
            return {new_normal, new_e1, new_e2};
        }
    } // namespace

    CircleArc CircleArc::normalized() const
    {
        auto const [new_normal, new_e1, new_e2] = normalize_base(e1_, e2, normal_, "CircleArc");
        return {id_, center_, new_normal, new_e1, new_e2, radius_, angle_span_};
    }

    CircleArc CircleArc::rotate(double angle) const
    {
        auto const new_e1 = std::cos(angle) * e1_ + std::sin(angle) * e2;
        return {id_, center_, normal_, new_e1, normal_.cross(new_e1), radius_, angle_span_};
    }

    Rectangle Rectangle::normalized() const
    {
        auto const [new_normal, new_e1, new_e2] = normalize_base(e1_, e2_, normal_, "Rectangle");
        return Rectangle{id_, center_, new_normal, new_e1, new_e2, width_, height_};
    }

    SphericalRectangle SphericalRectangle::normalized() const
    {
        auto const [new_normal, new_e1, new_e2] = normalize_base(e1_, e2_, normal_, "SphericalRectangle");
        return SphericalRectangle{id_, center_, new_normal, new_e1, new_e2, radius_, polar_span_, azimuth_span_};
    }

    pos_t SphericalRectangle::pos_at(double t1, double t2) const
    {
        // Map to azimuthal angle offset: phi in [-azimuth/2, azimuth/2]
        double const azimuth = (t1 - 0.5) * azimuth_span_;
        double const sin_azimuth = std::sin(azimuth);
        double const cos_azimuth = std::cos(azimuth);

        // Map to polar angle offset: theta in [-polar/2, polar/2]
        double const polar = (t2 - 0.5) * polar_span_;
        double const sin_polar = std::sin(polar);
        double const cos_polar = std::cos(polar);

        // Compute local unit vector on the sphere's surface relative to the sphere center
        auto const local_normal = cos_polar * cos_azimuth * normal_ + cos_polar * sin_azimuth * e1_ + sin_polar * e2_;

        // Project outward to the sphere's surface
        return center_ + radius_ * local_normal;
    }

    template <any_json_t JsonType>
    void to_json(JsonType& js, Line const& l)
    {
        js = JsonType{
            {"id", l.id_},
            {"pos1", l.pos1_},
            {"pos2", l.pos2_} //
        };
    }

    template <any_json_t JsonType>
    void from_json(JsonType const& js, Line& l)
    {
        serialization::assert_structure(js,
            "geometry::Rectangle",
            {
                {"id", json::value_t::string},
                {"pos1", json::value_t::array},
                {"pos2", json::value_t::array},
            },
            {});
        reconstruct_at(l,
            Line{
                js.at("id").template get<std::string>(),
                js.at("pos1").template get<pos_t>(),
                js.at("pos2").template get<pos_t>() //
            });
    }

    template <any_json_t JsonType>
    void to_json(JsonType& js, CircleArc const& c)
    {
        js = JsonType{
            {"id", c.id_},
            {"center", c.center_},
            {"normal", c.normal_},
            {"e1", c.e1_},
            {"e2", c.e2},
            {"radius", c.radius_},
            {"angle_span", c.angle_span_} //
        };
    }

    template <any_json_t JsonType>
    void from_json(JsonType const& js, CircleArc& c)
    {
        serialization::assert_structure(js,
            "geometry::CircleArc",
            {
                {"id", json::value_t::string},
                {"center", json::value_t::array},
                {"normal", json::value_t::array},
                {"e1", json::value_t::array},
                {"radius", json::value_t::number_float},
                {"angle_span", json::value_t::number_float},
            },
            {{"e2", json::value_t::array}});
        reconstruct_at(c,
            CircleArc{js.at("id").template get<std::string>(),
                js.at("center").template get<pos_t>(),
                js.at("normal").template get<pos_t>(),
                js.at("e1").template get<pos_t>(),
                POS_ZERO,
                js.at("radius").template get<double>(),
                js.at("angle_span").template get<double>()}
                .normalized());
    }

    template <any_json_t JsonType>
    void to_json(JsonType& js, Rectangle const& r)
    {
        js = JsonType{
            {"id", r.id_},
            {"center", r.center_},
            {"normal", r.normal_},
            {"e1", r.e1_},
            {"e2", r.e2_},
            {"width", r.width_},
            {"height", r.height_} //
        };
    }

    template <any_json_t JsonType>
    void from_json(JsonType const& js, Rectangle& r)
    {
        serialization::assert_structure(js,
            "geometry::Rectangle",
            {
                {"id", json::value_t::string},
                {"center", json::value_t::array},
                {"normal", json::value_t::array},
                {"e1", json::value_t::array},
                {"width", json::value_t::number_float},
                {"height", json::value_t::number_float},
            },
            {{"e2", json::value_t::array}});
        reconstruct_at(r,
            Rectangle{js.at("id").template get<std::string>(),
                js.at("center").template get<pos_t>(),
                js.at("normal").template get<pos_t>(),
                js.at("e1").template get<pos_t>(),
                POS_ZERO,
                js.at("width").template get<double>(),
                js.at("height").template get<double>()}
                .normalized());
    }

    template <any_json_t JsonType>
    void to_json(JsonType& js, SphericalRectangle const& sr)
    {
        js = JsonType{
            {"id", sr.id_},
            {"center", sr.center_},
            {"normal", sr.normal_},
            {"e1", sr.e1_},
            {"e2", sr.e2_},
            {"radius", sr.radius_},
            {"polar_span", sr.polar_span_},
            {"azimuth_span", sr.azimuth_span_} //
        };
    }

    template <any_json_t JsonType>
    void from_json(JsonType const& js, SphericalRectangle& sr)
    {
        serialization::assert_structure(js,
            "geometry::SphericalRectangle",
            {
                {"id", json::value_t::string},
                {"center", json::value_t::array},
                {"normal", json::value_t::array},
                {"e1", json::value_t::array},
                {"radius", json::value_t::number_float},
                {"polar_span", json::value_t::number_float},
                {"azimuth_span", json::value_t::number_float},
            },
            {{"e2", json::value_t::array}});
        reconstruct_at(sr,
            SphericalRectangle(js.at("id").template get<std::string>(),
                js.at("center").template get<pos_t>(),
                js.at("normal").template get<pos_t>(),
                js.at("e1").template get<pos_t>(),
                POS_ZERO,
                js.at("radius").template get<double>(),
                js.at("polar_span").template get<double>(),
                js.at("azimuth_span").template get<double>())
                .normalized());
    }

    Geometry& get(std::span<Geometry> geometries, std::string const& id)
    {
        auto const it = std::ranges::find(geometries, id, [](auto& geo) { return std::visit([](auto& g) { return g.id; }, geo); });
        if (it == geometries.end()) { throw SimulationError("Could not find geometry with id '{}'", id); }
        return *it;
    }
} // namespace geometry

// Line Instantiations
template void geometry::to_json(nlohmann::json&, geometry::Line const&);
template void geometry::to_json(nlohmann::ordered_json&, geometry::Line const&);
template void geometry::from_json(nlohmann::json const&, geometry::Line&);
template void geometry::from_json(nlohmann::ordered_json const&, geometry::Line&);

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
