//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "reference.hpp"
#include "testutil.hpp"

TEST_CASE("reference with default orientation", "[Reference]")
{
    reference::Reference const reference("ref1", "", Pos{1, 2, 3}, Quaternion{0, 0, 0});
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos), POS_ZERO); // this should always be the case
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos + Pos{1, 0, 0}), Pos(1, 0, 0));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos + Pos{0, 1, 0}), Pos(0, 1, 0));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos + Pos{0, 0, 1}), Pos(0, 0, 1));
}

TEST_CASE("reference with simple yaw", "[Reference]")
{
    reference::Reference const reference("ref1", "", Pos{0, 0, 0}, Quaternion{0, 0, pi / 2});
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos), POS_ZERO); // this should always be the case
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos + Pos{1, 0, 0}), Pos(0, -1, 0));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos + Pos{0, 1, 0}), Pos(1, 0, 0));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos + Pos{0, 0, 1}), Pos(0, 0, 1));
}

TEST_CASE("reference with simple pitch", "[Reference]")
{
    reference::Reference const reference("ref1", "", Pos{0, 0, 0}, Quaternion{0, pi / 2, 0});
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos), POS_ZERO); // this should always be the case
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(Pos{-1, 0, 0}), Pos(0, 0, -1));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(Pos{-1, -1, 0}), Pos(0, -1, -1));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(Pos{-1, 0, -1}), Pos(1, 0, -1));
}

TEST_CASE("reference with simple roll", "[Reference]")
{
    reference::Reference const reference("ref1", "", Pos{0, 0, 0}, Quaternion{pi / 4, 0, 0});
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos), POS_ZERO); // this should always be the case
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(Pos{0, 0, -1}), Pos(0, -sqrt2_2, -sqrt2_2));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(Pos{2, 0, -1}), Pos(2, -sqrt2_2, -sqrt2_2));
}

TEST_CASE("reference with yaw and pitch", "[Reference]")
{
    reference::Reference const reference("ref1", "", Pos{0, 0, 0}, Quaternion{0, pi / 4, pi / 2});
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos), POS_ZERO); // this should always be the case
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(Pos{-1, 0, 0}), Pos(0, 1, 0));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(Pos{0, -1, 0}), Pos(-sqrt2_2, 0, -sqrt2_2));
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(Pos{0, 0, -1}), Pos(sqrt2_2, 0, -sqrt2_2));
}

TEST_CASE("cascaded references without rotation", "[CascadedReferences]")
{
    reference::Reference ref1("ref1", "", Pos{1, 0, 0}, Quaternion{0, 0, 0});
    reference::Reference ref2("ref2", "ref1", Pos{0, 1, 0}, Quaternion{0, 0, 0});
    reference::Reference ref3("ref3", "ref2", Pos{0, 0, 1}, Quaternion{0, 0, 0});
    reference::resolve_origins({ref1, ref2, ref3});
    test_basic_transformations(ref1);
    test_basic_transformations(ref2);
    test_basic_transformations(ref3);
    REQUIRE_CLOSE_POSITION(ref3.global_from_local_pos(Pos{1, 2, 3}), Pos(2, 3, 4));
    REQUIRE_CLOSE_POSITION(ref3.global_from_local_pos(Pos{-1, -1, -1}), POS_ZERO);
}

TEST_CASE("cascaded references with rotation", "[CascadedReferences]")
{
    reference::Reference ref1("ref1", "", Pos{1, 0, 0}, Quaternion{0, 0, pi / 2});
    reference::Reference ref2("ref2", "ref1", Pos{1, 0, 0}, Quaternion{0, -pi / 2, 0});
    reference::Reference ref3("ref3", "ref2", Pos{1, 0, 0}, Quaternion{-pi / 2, 0, -pi / 2});
    reference::resolve_origins({ref1, ref2, ref3});
    test_basic_transformations(ref1);
    test_basic_transformations(ref2);
    test_basic_transformations(ref3);
    REQUIRE_CLOSE_POSITION(ref3.global_from_local_pos(Pos{1, 2, 3}), Pos(2, 3, 4));
    REQUIRE_CLOSE_POSITION(ref3.global_from_local_pos(Pos{-1, -1, -1}), POS_ZERO);
}
