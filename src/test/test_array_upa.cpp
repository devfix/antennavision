//
// Created by Tristan Krause on 2026-08-18.
//
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <print>
#include "components/antenna.hpp"
#include "context.hpp"
#include "three.hpp"

/*
 * Note: Instead of checking a computed field quantity, this test focuses on the correct construction of the radiator array only.
 * The correct numerical computations are checked in dedicated test files since the only take a set of placed radiators regardless of the array type etc.
 */

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using namespace components;
using reference::Reference;
constexpr double wavelength = 1.0;
constexpr std::size_t size_x = 5;
constexpr std::size_t size_y = 4;
constexpr double epsilon = 1e-6;
constexpr double delta = 1e-6;

constexpr std::array<std::array<Pos, size_x>, size_y> expected_pos = {{
    {
        Pos(-0.000, -2.000, 0.250),
        Pos(-0.000, -1.000, 0.250),
        Pos(0.000, 0.000, 0.250),
        Pos(0.000, 1.000, 0.250),
        Pos(0.000, 2.000, 0.250), //
    },
    {
        Pos(-0.000, -2.000, 0.750),
        Pos(-0.000, -1.000, 0.750),
        Pos(0.000, 0.000, 0.750),
        Pos(0.000, 1.000, 0.750),
        Pos(0.000, 2.000, 0.750), //
    },
    {
        Pos(-0.000, -2.000, 1.250),
        Pos(-0.000, -1.000, 1.250),
        Pos(-0.000, 0.000, 1.250),
        Pos(0.000, 1.000, 1.250),
        Pos(0.000, 2.000, 1.250), //
    },
    {
        Pos(-0.000, -2.000, 1.750),
        Pos(-0.000, -1.000, 1.750),
        Pos(-0.000, 0.000, 1.750),
        Pos(0.000, 1.000, 1.750),
        Pos(0.000, 2.000, 1.750), //
    } //
}};

constexpr std::array<std::array<Pos, size_x>, size_y> expected_ex = {{
    {
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000), //
    },
    {
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000), //
    },
    {
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000), //
    },
    {
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000),
        Pos(-1.000, 0.000, -0.000), //
    } //
}};

constexpr std::array<std::array<Pos, size_x>, size_y> expected_ey = {{
    {
        Pos(-0.000, -0.000, 1.000),
        Pos(-0.000, 0.000, 1.000),
        Pos(-0.000, 0.000, 1.000),
        Pos(-0.000, 0.000, 1.000),
        Pos(-0.000, 0.000, 1.000), //
    },
    {
        Pos(-0.000, -0.000, 1.000),
        Pos(-0.000, 0.000, 1.000),
        Pos(-0.000, 0.000, 1.000),
        Pos(-0.000, 0.000, 1.000),
        Pos(-0.000, 0.000, 1.000), //
    },
    {
        Pos(-0.000, -0.000, 1.000),
        Pos(-0.000, 0.000, 1.000),
        Pos(-0.000, 0.000, 1.000),
        Pos(-0.000, 0.000, 1.000),
        Pos(-0.000, 0.000, 1.000), //
    },
    {
        Pos(-0.000, -0.000, 1.000),
        Pos(-0.000, 0.000, 1.000),
        Pos(-0.000, 0.000, 1.000),
        Pos(-0.000, 0.000, 1.000),
        Pos(-0.000, 0.000, 1.000), //
    } //
}};

constexpr std::array<std::array<Pos, size_x>, size_y> expected_ez = {{
    {
        Pos(0.000, 1.000, 0.000),
        Pos(0.000, 1.000, 0.000),
        Pos(0.000, 1.000, -0.000),
        Pos(0.000, 1.000, 0.000),
        Pos(0.000, 1.000, 0.000), //
    },
    {
        Pos(0.000, 1.000, 0.000),
        Pos(0.000, 1.000, 0.000),
        Pos(0.000, 1.000, -0.000),
        Pos(0.000, 1.000, 0.000),
        Pos(0.000, 1.000, 0.000), //
    },
    {
        Pos(0.000, 1.000, 0.000),
        Pos(0.000, 1.000, 0.000),
        Pos(0.000, 1.000, -0.000),
        Pos(0.000, 1.000, 0.000),
        Pos(0.000, 1.000, 0.000), //
    },
    {
        Pos(0.000, 1.000, 0.000),
        Pos(0.000, 1.000, 0.000),
        Pos(0.000, 1.000, -0.000),
        Pos(0.000, 1.000, 0.000),
        Pos(0.000, 1.000, 0.000), //
    } //
}};

TEST_CASE("UPA position and rotation", "[RadiatorArray][UPA]")
{
    Antenna tx = RadiatorArray::create({
        .type = RadiatorArray::Type::UniformPlanarArray,
        .id = "tx",
        .origin_id = "ref",
        .rot = {0, pi / 2, 0},
        .prototype_desc =
            {
                .type = Radiator::Type::IsotropicRadiator,
            },
        .parameters =
            RadiatorArray::UniformPlanarParameters{
                .spacing_x = 1 * wavelength,
                .spacing_y = 0.5 * wavelength,
                .size_x = size_x,
                .size_y = size_y //
            } //
    });
    auto ref = Reference::create( //
        "ref",
        "",
        {0, 0, 1},
        {pi / 2, 0, pi / 2} // x->y, y->z, z->x
    ); //
    antenna::rebind_origin_pointers({tx}, {ref});

    auto& arr = antenna::cast<RadiatorArray>(tx);
    REQUIRE(antenna::size(tx) == size_x * size_y);
    REQUIRE(arr.elements.size() == size_x * size_y);
    for (std::size_t y = 0; y < size_y; y++)
    {
        for (std::size_t x = 0; x < size_x; x++)
        {
            CAPTURE(x, y);
            auto& origin = arr.get_origin(x, y);
            auto const pos = origin.global_from_local_pos(POS_ZERO);
            auto const ex = origin.global_from_local_pos({1, 0, 0}) - pos;
            auto const ey = origin.global_from_local_pos({0, 1, 0}) - pos;
            auto const ez = origin.global_from_local_pos({0, 0, 1}) - pos;
            CHECK_THAT(pos.x, WithinAbs(expected_pos.at(y).at(x).x, delta));
            CHECK_THAT(pos.y, WithinAbs(expected_pos.at(y).at(x).y, delta));
            CHECK_THAT(pos.z, WithinAbs(expected_pos.at(y).at(x).z, delta));
            CHECK_THAT(ex.x, WithinAbs(expected_ex.at(y).at(x).x, delta));
            CHECK_THAT(ex.y, WithinAbs(expected_ex.at(y).at(x).y, delta));
            CHECK_THAT(ex.z, WithinAbs(expected_ex.at(y).at(x).z, delta));
            CHECK_THAT(ey.x, WithinAbs(expected_ey.at(y).at(x).x, delta));
            CHECK_THAT(ey.y, WithinAbs(expected_ey.at(y).at(x).y, delta));
            CHECK_THAT(ey.z, WithinAbs(expected_ey.at(y).at(x).z, delta));
            CHECK_THAT(ez.x, WithinAbs(expected_ez.at(y).at(x).x, delta));
            CHECK_THAT(ez.y, WithinAbs(expected_ez.at(y).at(x).y, delta));
            CHECK_THAT(ez.z, WithinAbs(expected_ez.at(y).at(x).z, delta));
        }
    }
}
