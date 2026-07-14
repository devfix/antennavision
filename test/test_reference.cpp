//
// Created by Tristan Krause on 2026-05-26.
//
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "reference.hpp"
#include "testutil.hpp"


TEST_CASE("reference with default orientation", "[Reference]")
{
    Reference const reference("ref1", nullptr, pos_t{1, 2, 3}, Quaternion{0, 0, 0});
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos), POS_ZERO); // this should always be the case
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos + pos_t{1, 0, 0}), pos_t(1, 0, 0));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos + pos_t{0, 1, 0}), pos_t(0, 1, 0));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos + pos_t{0, 0, 1}), pos_t(0, 0, 1));
}

TEST_CASE("reference with simple yaw", "[Reference]")
{
    Reference const reference("ref1", nullptr, pos_t{0, 0, 0}, Quaternion{0, 0, pi / 2});
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos), POS_ZERO); // this should always be the case
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos + pos_t{1, 0, 0}), pos_t(0, -1, 0));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos + pos_t{0, 1, 0}), pos_t(1, 0, 0));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos + pos_t{0, 0, 1}), pos_t(0, 0, 1));
}

TEST_CASE("reference with simple pitch", "[Reference]")
{
    Reference const reference("ref1", nullptr, pos_t{0, 0, 0}, Quaternion{0, pi / 2, 0});
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos), POS_ZERO); // this should always be the case
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(pos_t{-1, 0, 0}), pos_t(0, 0, -1));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(pos_t{-1, -1, 0}), pos_t(0, -1, -1));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(pos_t{-1, 0, -1}), pos_t(1, 0, -1));
}

TEST_CASE("reference with simple roll", "[Reference]")
{
    Reference const reference("ref1", nullptr, pos_t{0, 0, 0}, Quaternion{pi / 4, 0, 0});
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos), POS_ZERO); // this should always be the case
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(pos_t{0, 0, -1}), pos_t(0, -sqrt2_2, -sqrt2_2));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(pos_t{2, 0, -1}), pos_t(2, -sqrt2_2, -sqrt2_2));
}

TEST_CASE("reference with yaw and pitch", "[Reference]")
{
    Reference const reference("ref1", nullptr, pos_t{0, 0, 0}, Quaternion{0, pi / 4, pi / 2});
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos), POS_ZERO); // this should always be the case
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(pos_t{-1, 0, 0}), pos_t(0, 1, 0));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(pos_t{0, -1, 0}), pos_t(-sqrt2_2, 0, -sqrt2_2));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(pos_t{0, 0, -1}), pos_t(sqrt2_2, 0, -sqrt2_2));
}


TEST_CASE("cascaded references without rotation", "[CascadedReferences]")
{
    Reference ref1("ref1", nullptr, pos_t{1, 0, 0}, Quaternion{0, 0, 0});
    Reference ref2("ref2", &ref1, pos_t{0, 1, 0}, Quaternion{0, 0, 0});
    Reference ref3("ref3", &ref2, pos_t{0, 0, 1}, Quaternion{0, 0, 0});
    test_basic_transformations(ref1);
    test_basic_transformations(ref2);
    test_basic_transformations(ref3);
    REQUIRE_CLOSE_POSITION(ref3.global_from_local_pos(pos_t{1, 2, 3}), pos_t(2, 3, 4));
    REQUIRE_CLOSE_POSITION(ref3.global_from_local_pos(pos_t{-1, -1, -1}), POS_ZERO);
}

TEST_CASE("cascaded references with rotation", "[CascadedReferences]")
{
    Reference ref1("ref1", nullptr, pos_t{1, 0, 0}, Quaternion{0, 0, pi/2});
    Reference ref2("ref2", &ref1, pos_t{1, 0, 0}, Quaternion{0, -pi/2, 0});
    Reference ref3("ref3", &ref2, pos_t{1, 0, 0}, Quaternion{-pi/2, 0, -pi/2});
    test_basic_transformations(ref1);
    test_basic_transformations(ref2);
    test_basic_transformations(ref3);
    REQUIRE_CLOSE_POSITION(ref3.global_from_local_pos(pos_t{1, 2, 3}), pos_t(2, 3, 4));
    REQUIRE_CLOSE_POSITION(ref3.global_from_local_pos(pos_t{-1, -1, -1}), POS_ZERO);
}
