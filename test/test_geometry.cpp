//
// Created by core on 2026-07-22.
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <nlohmann/json.hpp>

#include "geometry.hpp"
#include "simulationerror.hpp"

using Catch::Matchers::WithinAbs;

TEST_CASE("CircleArc JSON Serialization and Deserialization", "[geometry][json]")
{
    SECTION("Serialization outputs expected fields")
    {
        geometry::CircleArc const arc{
            .id_ = "arc_01",
            .center_ = {1.0, 2.0, 3.0},
            .normal_ = {0.0, 0.0, 1.0},
            .e1_ = {1.0, 0.0, 0.0},
            .e2 = {0.0, 1.0, 0.0},
            .radius_ = 5.0,
            .angle_span_ = 3.14159
        };

        nlohmann::json const js = arc;

        CHECK(js.at("center") == arc.center_);
        CHECK(js.at("normal") == arc.normal_);
        CHECK(js.at("e1") == arc.e1_);
        CHECK(js.at("e2") == arc.e2);
        CHECK(js.at("radius") == arc.radius_);
        CHECK(js.at("angle_span") == arc.angle_span_);
    }

    SECTION("Deserialization ignores input e2 and recomputes it via normal.cross(e1)")
    {
        // Provide a deliberately wrong/garbage e2 in JSON
        nlohmann::json const js = {
            {"id", "arc_01"},
            {"center", {0.0, 0.0, 0.0}},
            {"normal", {0.0, 0.0, 1.0}},
            {"e1", {1.0, 0.0, 0.0}},
            {"e2", {999.0, 999.0, 999.0}}, // Dummy value (should be ignored)
            {"radius", 2.5},
            {"angle_span", 1.57}
        };

        auto const arc = js.get<geometry::CircleArc>();

        CHECK(arc.id_ == "arc_01");
        CHECK(arc.radius_ == 2.5);
        CHECK(arc.angle_span_ == 1.57);

        // e2 must be reconstructed as normal.cross(e1) = (0, 0, 1) x (1, 0, 0) = (0, 1, 0)
        CHECK_THAT(arc.e2.x, WithinAbs(0.0, 1e-9));
        CHECK_THAT(arc.e2.y, WithinAbs(1.0, 1e-9));
        CHECK_THAT(arc.e2.z, WithinAbs(0.0, 1e-9));
    }

    SECTION("Deserialization throws when normal and e1 are not orthogonal")
    {
        nlohmann::json const js = {
            {"id", "arc_invalid"},
            {"center", {0.0, 0.0, 0.0}},
            {"normal", {0.0, 0.0, 1.0}},
            {"e1", {0.0, 0.0, 1.0}}, // Parallel to normal!
            {"radius", 1.0},
            {"angle_span", 1.0}
        };

        REQUIRE_THROWS_AS(js.get<geometry::CircleArc>(), SimulationError);
    }
}

TEST_CASE("Rectangle JSON Serialization and Deserialization", "[geometry][json]")
{
    SECTION("Round-trip serialization preserves values")
    {
        geometry::Rectangle const rect{
            .id_ = "rect_01",
            .center_ = {0.0, 0.0, 0.0},
            .normal_ = {0.0, 1.0, 0.0},
            .e1_ = {0.0, 0.0, 1.0},
            .e2_ = {1.0, 0.0, 0.0},
            .width_ = 10.0,
            .height_ = 20.0
        };
        nlohmann::json const js = rect;

        auto const deserialized = js.get<geometry::Rectangle>();
        CHECK(deserialized.id_ == rect.id_);
        CHECK(deserialized.width_ == rect.width_);
        CHECK(deserialized.height_ == rect.height_);
        CHECK(deserialized.normal_ == rect.normal_);
        CHECK(deserialized.e1_ == rect.e1_);
        CHECK(deserialized.e2_ == rect.e2_);
    }

    SECTION("Deserialization ignores input e2 and recomputes it")
    {
        nlohmann::json const js = {
            {"id", "rect_02"},
            {"center", {1.0, 1.0, 1.0}},
            {"normal", {0.0, 0.0, 1.0}},
            {"e1", {0.0, 1.0, 0.0}},
            {"e2", {-50.0, 0.0, 0.0}}, // Ignore wrong vector
            {"width", 4.0},
            {"height", 8.0}
        };

        auto const rect = js.get<geometry::Rectangle>();

        // e2 = normal.cross(e1) = (0,0,1) x (0,1,0) = (-1, 0, 0)
        CHECK_THAT(rect.e2_.x, WithinAbs(-1.0, 1e-9));
        CHECK_THAT(rect.e2_.y, WithinAbs(0.0, 1e-9));
        CHECK_THAT(rect.e2_.z, WithinAbs(0.0, 1e-9));
    }
}

TEST_CASE("SphericalRectangle JSON Serialization and Deserialization", "[geometry][json]")
{
    SECTION("Deserialization reconstructs e2 and normalizes vectors")
    {
        // Un-normalized vectors
        nlohmann::json const js = {
            {"id", "sph_01"},
            {"center", {0.0, 0.0, 0.0}},
            {"normal", {0.0, 0.0, 2.0}}, // Unnormalized
            {"e1", {3.0, 0.0, 0.0}},     // Unnormalized
            {"radius", 10.0},
            {"polar_span", 0.5},
            {"azimuth_span", 1.0}
        };

        auto const sr = js.get<geometry::SphericalRectangle>();

        CHECK(sr.id_ == "sph_01");
        CHECK(sr.radius_ == 10.0);
        CHECK(sr.polar_span_ == 0.5);
        CHECK(sr.azimuth_span_ == 1.0);

        // Verify normal and e1 were unit-normalized
        CHECK_THAT(sr.normal_.norm(), WithinAbs(1.0, 1e-9));
        CHECK_THAT(sr.e1_.norm(), WithinAbs(1.0, 1e-9));

        // Verify e2 was recomputed and unit-length
        // (0,0,1) x (1,0,0) = (0,1,0)
        CHECK_THAT(sr.e2_.x, WithinAbs(0.0, 1e-9));
        CHECK_THAT(sr.e2_.y, WithinAbs(1.0, 1e-9));
        CHECK_THAT(sr.e2_.z, WithinAbs(0.0, 1e-9));
    }
}
