//
// Created by core on 2026-07-14.
//

#include "geometry.hpp"
#include <nlohmann/json.hpp>
#include "simulationerror.hpp"

namespace nc {
    template <typename BasicJsonType>
    void to_json(BasicJsonType& j, pos_t const& v) {
        j = nlohmann::json{ v.x, v.y, v.z };
    }

    template <typename BasicJsonType>
    void from_json(BasicJsonType const& j, pos_t& v) {
        // If the JSON is a 3-element array [x, y, z]
        if (j.is_array() && j.size() == 3) {
            v.x = j.at(0).template get<double>();
            v.y = j.at(1).template get<double>();
            v.z = j.at(2).template get<double>();
        } else {
            throw nlohmann::json::type_error::create(302, "Validation failed: expected a 3-element array for nc::Vec3", &j);
        }
    }
} // namespace nc

namespace geometry
{
    template <typename BasicJsonType>
    void to_json(BasicJsonType& j, Circle const& sr) {
        j = nlohmann::json{
                {"center",  sr.center},
                {"normal",  sr.normal},
                {"radius",  sr.radius},
                {"e1",      sr.e1},
                {"e2",      sr.e2}
        };
    }

    template <typename BasicJsonType>
    void from_json(BasicJsonType const& j, Circle& sr) {
        j.at("center").get_to(sr.center);
        j.at("normal").get_to(sr.normal);
        j.at("radius").get_to(sr.radius);
        j.at("e1").get_to(sr.e1);
        j.at("e2").get_to(sr.e2);
    }

    template <typename BasicJsonType>
    void to_json(BasicJsonType& j, Rectangle const& sr) {
        j = nlohmann::json{
                {"center",  sr.center},
                {"normal",  sr.normal},
                {"width",  sr.width},
                {"height",  sr.height},
                {"e1",      sr.e1},
                {"e2",      sr.e2}
        };
    }

    template <typename BasicJsonType>
    void from_json(BasicJsonType const& j, Rectangle& sr) {
        j.at("center").get_to(sr.center);
        j.at("normal").get_to(sr.normal);
        j.at("width").get_to(sr.width);
        j.at("height").get_to(sr.height);
        j.at("e1").get_to(sr.e1);
        j.at("e2").get_to(sr.e2);
    }

    template <typename BasicJsonType>
    void to_json(BasicJsonType& j, SphericalRectangle const& sr) {
        j = nlohmann::json{
            {"center",  sr.center},
            {"normal",  sr.normal},
            {"radius",  sr.radius},
            {"polar",   sr.polar},
            {"azimuth", sr.azimuth},
            {"e1",      sr.e1},
            {"e2",      sr.e2}
        };
    }

    template <typename BasicJsonType>
    void from_json(BasicJsonType const& j, SphericalRectangle& sr) {
        j.at("center").get_to(sr.center);
        j.at("normal").get_to(sr.normal);
        j.at("radius").get_to(sr.radius);
        j.at("polar").get_to(sr.polar);
        j.at("azimuth").get_to(sr.azimuth);
        j.at("e1").get_to(sr.e1);
        j.at("e2").get_to(sr.e2);
    }

    Circle Circle::make(pos_t const& center, pos_t const& normal, double radius, pos_t const& dir_start)
    {
        pos_t const safe_normal = normal.normalize();
        // Project start_direction onto the plane to ensure it's perfectly perpendicular to the normal Vector projection: e1 = v - (v . n) * n
        pos_t const e1 = dir_start - dir_start.dot(safe_normal) * safe_normal;
        if (e1.norm() <= NUMERICAL_MARGIN) {throw SimulationError("Invalid circle start direction");}
        Circle circle{.center = center, .normal = safe_normal, .radius = radius, .e1 = e1.normalize()};
        circle.e2 = safe_normal.cross(circle.e1);
        return circle;
    }

    Circle Circle::rotate_base(double angle) const
    {
        pos_t const e1_new = std::cos(angle) * e1 + std::sin(angle) * e2;
        pos_t const e2_new = -std::sin(angle) * e1 + std::cos(angle) * e2;
        return {.center = center, .normal = normal, .radius = radius, .e1 = e1_new, .e2 = e2_new};
    }

    Rectangle Rectangle::make(pos_t const& pos_zero, pos_t const& pos_width_max, pos_t const& pos_height_max)
    {
        pos_t const dir_width = pos_width_max - pos_zero;
        pos_t const dir_height = pos_height_max - pos_zero;
        pos_t const normal = dir_width.cross(dir_height).normalize();
        pos_t const v1 = dir_width.normalize();
        return {.center = pos_zero + 0.5 * dir_width + 0.5 * dir_height,
                .normal = normal,
                .width = dir_width.norm(),
                .height = dir_height.norm(),
                .e1 = v1,
                .e2 = normal.cross(v1)};
    }

    SphericalRectangle SphericalRectangle::make(pos_t const& center, pos_t const& pos_rect, double polar, double azimuth, pos_t const& dir_north)
    {
        SphericalRectangle sr{
            .center = center,
            .normal = pos_rect.normalize(),
            .radius = pos_rect.norm(),
            .polar = polar,
            .azimuth = azimuth,
        };
        // Project dir_up onto the plane defined by normal to ensure it's perfectly perpendicular
        pos_t const e2 = (dir_north - dir_north.dot(sr.normal) * sr.normal);
        if (e2.norm() <= NUMERICAL_MARGIN) {throw SimulationError("Invalid spherical rectangle north direction");}
        sr.e2 = e2.normalize();
        sr.e1 = sr.e2.cross(sr.normal);
        return sr;
    }
} // namespace geometry

// nc::Vec3 Instantiations
template void nc::to_json(nlohmann::json&, pos_t const&);
template void nc::to_json(nlohmann::ordered_json&, pos_t const&);
template void nc::from_json(nlohmann::json const&, pos_t&);
template void nc::from_json(nlohmann::ordered_json const&, pos_t&);

// Circle Instantiations
template void geometry::to_json(nlohmann::json&, geometry::Circle const&);
template void geometry::to_json(nlohmann::ordered_json&, geometry::Circle const&);
template void geometry::from_json(nlohmann::json const&, geometry::Circle&);
template void geometry::from_json(nlohmann::ordered_json const&, geometry::Circle&);

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
