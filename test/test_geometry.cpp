//
// Created by Tristan Krause on 2026-07-22.
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <nlohmann/json.hpp>

#include "setup/geometry.hpp"
#include "simulationerror.hpp"

using Catch::Matchers::WithinAbs;
using geometry::Geometry;

TEST_CASE("Line Properties and JSON Serialization", "[geometry][line][json]")
{
    SECTION("Member functions compute correct length and positions")
    {
        geometry::Line const line{"line_01", {0.0, 0.0, 0.0}, {3.0, 4.0, 0.0}};

        CHECK(line.id() == "line_01");
        CHECK_THAT(line.length(), WithinAbs(5.0, 1e-9));

        // Test position interpolation along t in [0, 1]
        auto const start_pos = line.pos_at(0.0);
        CHECK_THAT(start_pos.x, WithinAbs(0.0, 1e-9));
        CHECK_THAT(start_pos.y, WithinAbs(0.0, 1e-9));
        CHECK_THAT(start_pos.z, WithinAbs(0.0, 1e-9));

        auto const mid_pos = line.pos_at(0.5);
        CHECK_THAT(mid_pos.x, WithinAbs(1.5, 1e-9));
        CHECK_THAT(mid_pos.y, WithinAbs(2.0, 1e-9));
        CHECK_THAT(mid_pos.z, WithinAbs(0.0, 1e-9));

        auto const end_pos = line.pos_at(1.0);
        CHECK_THAT(end_pos.x, WithinAbs(3.0, 1e-9));
        CHECK_THAT(end_pos.y, WithinAbs(4.0, 1e-9));
        CHECK_THAT(end_pos.z, WithinAbs(0.0, 1e-9));
    }

    SECTION("Round-trip serialization preserves values")
    {
        geometry::Line const line{"line_01", {1.0, -2.0, 3.5}, {4.0, 2.0, 3.5}};

        nlohmann::json const js = Geometry(line);

        CHECK(js.at("type") == "Line");
        CHECK(js.at("id") == line.id());
        CHECK(js.at("pos_begin") == line.pos_begin());
        CHECK(js.at("pos_end") == line.pos_end());

        auto const deserialized = std::get<geometry::Line>(js.get<Geometry>());

        CHECK(deserialized.id() == line.id());
        CHECK_THAT(deserialized.pos_begin().x, WithinAbs(1.0, 1e-9));
        CHECK_THAT(deserialized.pos_begin().y, WithinAbs(-2.0, 1e-9));
        CHECK_THAT(deserialized.pos_begin().z, WithinAbs(3.5, 1e-9));
        CHECK_THAT(deserialized.pos_end().x, WithinAbs(4.0, 1e-9));
        CHECK_THAT(deserialized.pos_end().y, WithinAbs(2.0, 1e-9));
        CHECK_THAT(deserialized.pos_end().z, WithinAbs(3.5, 1e-9));
    }

    SECTION("Deserialization throws on missing or invalid structure")
    {
        // Missing 'pos_end'
        nlohmann::json const js_missing = {//
            {"type", "Line"},
            {"id", "line_invalid"},
            {
                "pos_begin",
                {0.0, 0.0, 0.0} //
            }};

        REQUIRE_THROWS(js_missing.get<Geometry>());
    }
}

TEST_CASE("CircleArc JSON Serialization and Deserialization", "[geometry][json]")
{
    SECTION("Serialization outputs expected fields")
    {
        geometry::CircleArc const arc{"arc_01", {1.0, 2.0, 3.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 5.0, 3.14159};

        nlohmann::json const js = Geometry(arc);

        CHECK(js.at("type") == "CircleArc");
        CHECK(js.at("center") == arc.center());
        CHECK(js.at("normal") == arc.normal());
        CHECK(js.at("e1") == arc.e1());
        CHECK(js.at("e2") == arc.e2());
        CHECK(js.at("radius") == arc.radius());
        CHECK(js.at("angle_span") == arc.angle_span());
    }

    SECTION("Deserialization ignores input e2 and recomputes it via normal.cross(e1)")
    {
        // Provide a deliberately wrong/garbage e2 in JSON
        nlohmann::json const js = {//
            {"type", "CircleArc"},
            {"id", "arc_01"},
            {"center", {0.0, 0.0, 0.0}},
            {"normal", {0.0, 0.0, 1.0}},
            {"e1", {1.0, 0.0, 0.0}},
            {"e2", {999.0, 999.0, 999.0}}, // Dummy value (should be ignored)
            {"radius", 2.5},
            {"angle_span", 1.57} //
        };

        auto const arc = std::get<geometry::CircleArc>(js.get<Geometry>());

        CHECK(arc.id() == "arc_01");
        CHECK(arc.radius() == 2.5);
        CHECK(arc.angle_span() == 1.57);

        // e2 must be reconstructed as normal.cross(e1) = (0, 0, 1) x (1, 0, 0) = (0, 1, 0)
        CHECK_THAT(arc.e2().x, WithinAbs(0.0, 1e-9));
        CHECK_THAT(arc.e2().y, WithinAbs(1.0, 1e-9));
        CHECK_THAT(arc.e2().z, WithinAbs(0.0, 1e-9));
    }

    SECTION("Deserialization throws when normal and e1 are not orthogonal")
    {
        nlohmann::json const js = {//
            {"type", "CircleArc"},
            {"id", "arc_invalid"},
            {"center", {0.0, 0.0, 0.0}},
            {"normal", {0.0, 0.0, 1.0}},
            {"e1", {0.0, 0.0, 1.0}}, // Parallel to normal!
            {"radius", 1.0},
            {"angle_span", 1.0}//
        };

        REQUIRE_THROWS_AS(js.get<Geometry>(), SimulationError);
    }
}

TEST_CASE("Rectangle JSON Serialization and Deserialization", "[geometry][json]")
{
    SECTION("Round-trip serialization preserves values")
    {
        geometry::Rectangle const rect{"rect_01", {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 0.0}, 10.0, 20.0};
        nlohmann::json const js = Geometry(rect);

        auto const deserialized = std::get<geometry::Rectangle>(js.get<Geometry>());
        CHECK(deserialized.id() == rect.id());
        CHECK(deserialized.width() == rect.width());
        CHECK(deserialized.height() == rect.height());
        CHECK(deserialized.normal() == rect.normal());
        CHECK(deserialized.e1() == rect.e1());
        CHECK(deserialized.e2() == rect.e2());
    }

    SECTION("Deserialization ignores input e2 and recomputes it")
    {
        nlohmann::json const js = {//
            {"type", "Rectangle"},
            {"id", "rect_02"},
            {"center", {1.0, 1.0, 1.0}},
            {"normal", {0.0, 0.0, 1.0}},
            {"e1", {0.0, 1.0, 0.0}},
            {"e2", {-50.0, 0.0, 0.0}}, // Ignore wrong vector
            {"width", 4.0},
            {"height", 8.0}//
        };

        auto const rect = std::get<geometry::Rectangle>(js.get<Geometry>());

        // e2 = normal.cross(e1) = (0,0,1) x (0,1,0) = (-1, 0, 0)
        CHECK_THAT(rect.e2().x, WithinAbs(-1.0, 1e-9));
        CHECK_THAT(rect.e2().y, WithinAbs(0.0, 1e-9));
        CHECK_THAT(rect.e2().z, WithinAbs(0.0, 1e-9));
    }
}

TEST_CASE("SphericalRectangle JSON Serialization and Deserialization", "[geometry][json]")
{
    SECTION("Deserialization reconstructs e2 and normalizes vectors")
    {
        // Un-normalized vectors
        nlohmann::json const js = {//
            {"type", "SphericalRectangle"},
            {"id", "sph_01"},
            {"center", {0.0, 0.0, 0.0}},
            {"normal", {0.0, 0.0, 2.0}}, // Unnormalized
            {"e1", {3.0, 0.0, 0.0}}, // Unnormalized
            {"radius", 10.0},
            {"polar_span", 0.5},
            {"azimuth_span", 1.0}//
        };

        auto const sr = std::get<geometry::SphericalRectangle>(js.get<Geometry>());

        CHECK(sr.id() == "sph_01");
        CHECK(sr.radius() == 10.0);
        CHECK(sr.polar_span() == 0.5);
        CHECK(sr.azimuth_span() == 1.0);

        // Verify normal and e1 were unit-normalized
        CHECK_THAT(sr.normal().norm(), WithinAbs(1.0, 1e-9));
        CHECK_THAT(sr.e1().norm(), WithinAbs(1.0, 1e-9));

        // Verify e2 was recomputed and unit-length
        // (0,0,1) x (1,0,0) = (0,1,0)
        CHECK_THAT(sr.e2().x, WithinAbs(0.0, 1e-9));
        CHECK_THAT(sr.e2().y, WithinAbs(1.0, 1e-9));
        CHECK_THAT(sr.e2().z, WithinAbs(0.0, 1e-9));
    }
}
