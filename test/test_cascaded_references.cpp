//
// Created by Tristan Krause on 2026-05-26.
//
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "reference.hpp"
#include "testutil.hpp"

TEST_CASE("cascaded references without rotation", "[CascadedReferences]")
{
    Reference const ref1("ref1", nullptr, Vec3{1, 0, 0}, Quaternion{0, 0, 0});
    Reference const ref2("ref2", &ref1, Vec3{0, 1, 0}, Quaternion{0, 0, 0});
    Reference const ref3("ref3", &ref2, Vec3{0, 0, 1}, Quaternion{0, 0, 0});
    test_basic_transformations(ref1);
    test_basic_transformations(ref2);
    test_basic_transformations(ref3);
    require_close_position(ref3.global_from_local_pos(Vec3{1, 2, 3}), Vec3(2, 3, 4));
    require_close_position(ref3.global_from_local_pos(Vec3{-1, -1, -1}), POS_ZERO);
}

TEST_CASE("cascaded references with rotation", "[CascadedReferences]")
{
    Reference const ref1("ref1", nullptr, Vec3{1, 0, 0}, Quaternion{0, 0, pi/2});
    Reference const ref2("ref2", &ref1, Vec3{1, 0, 0}, Quaternion{0, -pi/2, 0});
    Reference const ref3("ref3", &ref2, Vec3{1, 0, 0}, Quaternion{-pi/2, 0, -pi/2});
    test_basic_transformations(ref1);
    test_basic_transformations(ref2);
    test_basic_transformations(ref3);
    require_close_position(ref3.global_from_local_pos(Vec3{1, 2, 3}), Vec3(2, 3, 4));
    require_close_position(ref3.global_from_local_pos(Vec3{-1, -1, -1}), POS_ZERO);
}
