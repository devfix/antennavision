//
// Created by Tristan Krause on 2026-07-14.
//

#include "setup/geometry.hpp"
#include <nlohmann/json.hpp>

#include "math.hpp"
#include "memory.hpp"
#include "serialization.hpp"
#include "simulationerror.hpp"

namespace geometry
{
    namespace
    {
        // Concept: Checks if type T belongs to the Curve variant
        template <typename T, typename Variant>
        concept is_variant_alternative = requires(T val) { Variant{val}; };

        void assert_orthogonality(Pos const& normal, Pos const& e1, Pos const& e2, std::string_view object)
        {
            if (std::abs(normal.dot(e1)) > NUMERICAL_MARGIN) { throw SimulationError("Invalid {} definition: normal and e1 must be orthogonal", object); }
            if (std::abs(normal.dot(e2)) > NUMERICAL_MARGIN) { throw SimulationError("Invalid {} definition: normal and e2 must be orthogonal", object); }
            if (std::abs(e1.dot(e2)) > NUMERICAL_MARGIN) { throw SimulationError("Invalid {} definition: e1 and e2 must be orthogonal", object); }
        }

        std::tuple<Pos, Pos, Pos> normalize_base(Pos const& e1, Pos const& e2, Pos const& normal, std::string_view object)
        {
            auto const new_normal = normal.normalize();
            auto const new_e1 = e1.normalize();
            auto const new_e2 = new_normal.cross(new_e1);
            assert_orthogonality(new_normal, new_e1, new_e2, object);
            return {new_normal, new_e1, new_e2};
        }

        template <any_json_t JsonType>
        void load_line(JsonType const& js, Geometry& g)
        {
            serialization::assert_structure(js,
                "geometry::Rectangle",
                {
                    {"type", json::value_t::string},
                    {"id", json::value_t::string},
                    {"pos_begin", json::value_t::array},
                    {"pos_end", json::value_t::array},
                },
                {});
            reconstruct_at(g,
                Line{
                    js.at("id").template get<std::string>(),
                    js.at("pos_begin").template get<Pos>(),
                    js.at("pos_end").template get<Pos>() //
                });
        }

        template <any_json_t JsonType>
        void load_circle_arc(JsonType const& js, Geometry& g)
        {
            serialization::assert_structure(js,
                "geometry::CircleArc",
                {
                    {"type", json::value_t::string},
                    {"id", json::value_t::string},
                    {"center", json::value_t::array},
                    {"normal", json::value_t::array},
                    {"e1", json::value_t::array},
                    {"radius", json::value_t::number_float},
                    {"angle_span", json::value_t::number_float},
                },
                {{"e2", json::value_t::array}});
            reconstruct_at(g,
                CircleArc{js.at("id").template get<std::string>(),
                    js.at("center").template get<Pos>(),
                    js.at("normal").template get<Pos>(),
                    js.at("e1").template get<Pos>(),
                    POS_ZERO,
                    js.at("radius").template get<double>(),
                    js.at("angle_span").template get<double>()}
                    .normalized());
        }

        template <any_json_t JsonType>
        void load_rectangle(JsonType const& js, Geometry& g)
        {
            serialization::assert_structure(js,
                "geometry::Rectangle",
                {
                    {"type", json::value_t::string},
                    {"id", json::value_t::string},
                    {"center", json::value_t::array},
                    {"normal", json::value_t::array},
                    {"e1", json::value_t::array},
                    {"width", json::value_t::number_float},
                    {"height", json::value_t::number_float},
                },
                {{"e2", json::value_t::array}});
            reconstruct_at(g,
                Rectangle{js.at("id").template get<std::string>(),
                    js.at("center").template get<Pos>(),
                    js.at("normal").template get<Pos>(),
                    js.at("e1").template get<Pos>(),
                    POS_ZERO,
                    js.at("width").template get<double>(),
                    js.at("height").template get<double>()}
                    .normalized());
        }

        template <any_json_t JsonType>
        void load_spherical_rectangle(JsonType const& js, Geometry& g)
        {
            serialization::assert_structure(js,
                "geometry::SphericalRectangle",
                {
                    {"type", json::value_t::string},
                    {"id", json::value_t::string},
                    {"center", json::value_t::array},
                    {"normal", json::value_t::array},
                    {"e1", json::value_t::array},
                    {"radius", json::value_t::number_float},
                    {"polar_span", json::value_t::number_float},
                    {"azimuth_span", json::value_t::number_float},
                },
                {{"e2", json::value_t::array}});
            reconstruct_at(g,
                SphericalRectangle(js.at("id").template get<std::string>(),
                    js.at("center").template get<Pos>(),
                    js.at("normal").template get<Pos>(),
                    js.at("e1").template get<Pos>(),
                    POS_ZERO,
                    js.at("radius").template get<double>(),
                    js.at("polar_span").template get<double>(),
                    js.at("azimuth_span").template get<double>())
                    .normalized());
        }
    } // namespace

    CircleArc CircleArc::normalized() const
    {
        auto const [new_normal, new_e1, new_e2] = normalize_base(e1_, e2_, normal_, "CircleArc");
        return {id_, center_, new_normal, new_e1, new_e2, radius_, angle_span_};
    }

    CircleArc CircleArc::rotate(double angle) const
    {
        auto const new_e1 = std::cos(angle) * e1_ + std::sin(angle) * e2_;
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

    Pos SphericalRectangle::pos_at(double t1, double t2) const
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
    void to_json(JsonType& js, Geometry const& geo)
    {
        if (Line const* l = std::get_if<Line>(&geo); l)
        {
            js = JsonType{
                {"type", "Line"},
                {"id", l->id()},
                {"pos_begin", l->pos_begin()},
                {"pos_end", l->pos_end()} //
            };
        }
        else if (CircleArc const* ca = std::get_if<CircleArc>(&geo); ca)
        {
            js = JsonType{
                {"type", "CircleArc"},
                {"id", ca->id()},
                {"center", ca->center()},
                {"normal", ca->normal()},
                {"e1", ca->e1()},
                {"e2", ca->e2()},
                {"radius", ca->radius()},
                {"angle_span", ca->angle_span()} //
            };
        }
        else if (Rectangle const* r = std::get_if<Rectangle>(&geo); r)
        {
            js = JsonType{
                {"type", "Rectangle"},
                {"id", r->id()},
                {"center", r->center()},
                {"normal", r->normal()},
                {"e1", r->e1()},
                {"e2", r->e2()},
                {"width", r->width()},
                {"height", r->height()} //
            };
        }
        else if (SphericalRectangle const* sr = std::get_if<SphericalRectangle>(&geo); sr)
        {
            js = JsonType{
                {"type", "SphericalRectangle"},
                {"id", sr->id()},
                {"center", sr->center()},
                {"normal", sr->normal()},
                {"e1", sr->e1()},
                {"e2", sr->e2()},
                {"radius", sr->radius()},
                {"polar_span", sr->polar_span()},
                {"azimuth_span", sr->azimuth_span()} //
            };
        }
        else
            throw SimulationError("Unknown geometry object");
    }

    template <any_json_t JsonType>
    void from_json(JsonType const& js, Geometry& geo)
    {
        if (!js.contains("type")) throw SimulationError("Missing geometry type");
        if (js.at("type").type() != nlohmann::json::value_t::string)
            throw SimulationError("Geometry attribute type must be string, but is {}", js.at("type").type_name());
        auto const type = js.at("type").template get<std::string>();

        if (type == "Line")
            load_line(js, geo);
        else if (type == "CircleArc")
            load_circle_arc(js, geo);
        else if (type == "Rectangle")
            load_rectangle(js, geo);
        else if (type == "SphericalRectangle")
            load_spherical_rectangle(js, geo);
        else
            throw SimulationError("Unknown geometry type '{}'", type);
    }

    Geometry& get(std::span<Geometry> geometries, std::string const& id)
    {
        auto const it = std::ranges::find(geometries, id, [](auto& geo) { return std::visit([](auto& g) { return g.id(); }, geo); });
        if (it == geometries.end()) { throw SimulationError("Could not find geometry with id '{}'", id); }
        return *it;
    }

    Vec3Array get_positions(Geometry const& geo, std::size_t n_linear1, std::size_t n_linear2)
    {
        return std::visit(
            [&n_linear1, &n_linear2]<typename T>(T const& shape) -> Vec3Array
            {
                if constexpr (is_variant_alternative<T, Curve>)
                {
                    Vec3Array positions(n_linear1, 1);
                    for (ComplexArray::index_type k = 0; k < n_linear1; k++)
                    {
                        double const t = static_cast<double>(k) / static_cast<double>(n_linear1 - 1);
                        positions(k, 0) = geometry::curve::get_pos_at(shape, t);
                    }
                    return positions;
                }
                else
                {
                    Vec3Array positions(n_linear2, n_linear1);
                    for (ComplexArray::index_type k2 = 0; k2 < n_linear2; k2++)
                    {
                        double const t2 = static_cast<double>(k2) / static_cast<double>(n_linear2 - 1);
                        for (RealArray::index_type k1 = 0; k1 < n_linear1; k1++)
                        {
                            double const t1 = static_cast<double>(k1) / static_cast<double>(n_linear1 - 1);
                            positions(k2, k1) = geometry::surface::get_pos_at(shape, t1, t2);
                        }
                    }
                    return positions;
                }
            },
            geo);
    }

    // Geometry Instantiations
    template void geometry::to_json(nlohmann::json&, Geometry const&);
    template void geometry::to_json(nlohmann::ordered_json&, Geometry const&);
    template void geometry::from_json(nlohmann::json const&, Geometry&);
    template void geometry::from_json(nlohmann::ordered_json const&, Geometry&);
} // namespace geometry
