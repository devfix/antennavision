//
// Created by Tristan Krause on 2026-05-26.
//
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "reference.hpp"
#include "testutil.hpp"

TEST_CASE("reference with default orientation", "[SingleReference]")
{
    Reference const reference("ref1", nullptr, Vec3{1, 2, 3}, Quaternion{0, 0, 0});
    require_close_position(reference.local_from_global(reference.pos), POS_ZERO); // this should always be the case
    require_close_position(reference.local_from_global(reference.pos + Vec3{1, 0, 0}), Vec3(1, 0, 0));
    require_close_position(reference.local_from_global(reference.pos + Vec3{0, 1, 0}), Vec3(0, 1, 0));
    require_close_position(reference.local_from_global(reference.pos + Vec3{0, 0, 1}), Vec3(0, 0, 1));
}

TEST_CASE("reference with simple yaw", "[SingleReference]")
{
    Reference const reference("ref1", nullptr, Vec3{0, 0, 0}, Quaternion{0, 0, pi / 2});
    require_close_position(reference.local_from_global(reference.pos), POS_ZERO); // this should always be the case
    require_close_position(reference.local_from_global(reference.pos + Vec3{1, 0, 0}), Vec3(0, -1, 0));
    require_close_position(reference.local_from_global(reference.pos + Vec3{0, 1, 0}), Vec3(1, 0, 0));
    require_close_position(reference.local_from_global(reference.pos + Vec3{0, 0, 1}), Vec3(0, 0, 1));
}

TEST_CASE("reference with simple pitch", "[SingleReference]")
{
    Reference const reference("ref1", nullptr, Vec3{0, 0, 0}, Quaternion{0, pi / 2, 0});
    require_close_position(reference.local_from_global(reference.pos), POS_ZERO); // this should always be the case
    require_close_position(reference.local_from_global(Vec3{-1, 0, 0}), Vec3(0, 0, -1));
    require_close_position(reference.local_from_global(Vec3{-1, -1, 0}), Vec3(0, -1, -1));
    require_close_position(reference.local_from_global(Vec3{-1, 0, -1}), Vec3(1, 0, -1));
}

TEST_CASE("reference with simple roll", "[SingleReference]")
{
    Reference const reference("ref1", nullptr, Vec3{0, 0, 0}, Quaternion{pi / 4, 0, 0});
    require_close_position(reference.local_from_global(reference.pos), POS_ZERO); // this should always be the case
    require_close_position(reference.local_from_global(Vec3{0, 0, -1}), Vec3(0, -sqrt2_2, -sqrt2_2));
    require_close_position(reference.local_from_global(Vec3{2, 0, -1}), Vec3(2, -sqrt2_2, -sqrt2_2));
}

TEST_CASE("reference with yaw and pitch", "[SingleReference]")
{
    Reference const reference("ref1", nullptr, Vec3{0, 0, 0}, Quaternion{0, pi / 4, pi / 2});
    require_close_position(reference.local_from_global(reference.pos), POS_ZERO); // this should always be the case
    require_close_position(reference.local_from_global(Vec3{-1, 0, 0}), Vec3(0, 1, 0));
    require_close_position(reference.local_from_global(Vec3{0, -1, 0}), Vec3(-sqrt2_2, 0, -sqrt2_2));
    require_close_position(reference.local_from_global(Vec3{0, 0, -1}), Vec3(sqrt2_2, 0, -sqrt2_2));
}
