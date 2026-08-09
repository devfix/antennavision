//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "../cmake-build-debug-local-static/_deps/catch2-src/src/catch2/matchers/catch_matchers.hpp"
#include "../cmake-build-release-local/_deps/catch2-src/src/catch2/matchers/catch_matchers_floating_point.hpp"
#include "reference.hpp"
#include "testutil.hpp"

using Catch::Matchers::WithinAbs;

TEST_CASE("reference with default orientation", "[Reference]")
{
    reference::Reference const reference("ref1", "", Pos{1, 2, 3}, Quaternion{0, 0, 0});
    CHECK_THAT((reference.local_from_global_pos(reference.pos) - POS_ZERO).norm(), WithinAbs(0, DELTA_DISTANCE)); // this should always be the case
    CHECK_THAT((reference.local_from_global_pos(reference.pos + Pos{1, 0, 0}) - Pos(1, 0, 0)).norm(), WithinAbs(0, DELTA_DISTANCE));
    CHECK_THAT((reference.local_from_global_pos(reference.pos + Pos{0, 1, 0}) - Pos(0, 1, 0)).norm(), WithinAbs(0, DELTA_DISTANCE));
    CHECK_THAT((reference.local_from_global_pos(reference.pos + Pos{0, 0, 1}) - Pos(0, 0, 1)).norm(), WithinAbs(0, DELTA_DISTANCE));
}

TEST_CASE("reference with simple yaw", "[Reference]")
{
    reference::Reference const reference("ref1", "", Pos{0, 0, 0}, Quaternion{0, 0, pi / 2});
    CHECK_THAT((reference.local_from_global_pos(reference.pos) - POS_ZERO).norm(), WithinAbs(0, DELTA_DISTANCE)); // this should always be the case
    CHECK_THAT((reference.local_from_global_pos(reference.pos + Pos{1, 0, 0}) - Pos(0, -1, 0)).norm(), WithinAbs(0, DELTA_DISTANCE));
    CHECK_THAT((reference.local_from_global_pos(reference.pos + Pos{0, 1, 0}) - Pos(1, 0, 0)).norm(), WithinAbs(0, DELTA_DISTANCE));
    CHECK_THAT((reference.local_from_global_pos(reference.pos + Pos{0, 0, 1}) - Pos(0, 0, 1)).norm(), WithinAbs(0, DELTA_DISTANCE));
}

TEST_CASE("reference with simple pitch", "[Reference]")
{
    reference::Reference const reference("ref1", "", Pos{0, 0, 0}, Quaternion{0, pi / 2, 0});
    CHECK_THAT((reference.local_from_global_pos(reference.pos) - POS_ZERO).norm(), WithinAbs(0, DELTA_DISTANCE)); // this should always be the case
    CHECK_THAT((reference.local_from_global_pos(Pos{-1, 0, 0}) - Pos(0, 0, -1)).norm(), WithinAbs(0, DELTA_DISTANCE));
    CHECK_THAT((reference.local_from_global_pos(Pos{-1, -1, 0}) - Pos(0, -1, -1)).norm(), WithinAbs(0, DELTA_DISTANCE));
    CHECK_THAT((reference.local_from_global_pos(Pos{-1, 0, -1}) - Pos(1, 0, -1)).norm(), WithinAbs(0, DELTA_DISTANCE));
}

TEST_CASE("reference with simple roll", "[Reference]")
{
    reference::Reference const reference("ref1", "", Pos{0, 0, 0}, Quaternion{pi / 4, 0, 0});
    CHECK_THAT((reference.local_from_global_pos(reference.pos) - POS_ZERO).norm(), WithinAbs(0, DELTA_DISTANCE)); // this should always be the case
    CHECK_THAT((reference.local_from_global_pos(Pos{0, 0, -1}) - Pos(0, -sqrt2_2, -sqrt2_2)).norm(), WithinAbs(0, DELTA_DISTANCE));
    CHECK_THAT((reference.local_from_global_pos(Pos{2, 0, -1}) - Pos(2, -sqrt2_2, -sqrt2_2)).norm(), WithinAbs(0, DELTA_DISTANCE));
}

TEST_CASE("reference with yaw and pitch", "[Reference]")
{
    reference::Reference const reference("ref1", "", Pos{0, 0, 0}, Quaternion{0, pi / 4, pi / 2});
    CHECK_THAT((reference.local_from_global_pos(reference.pos) - POS_ZERO).norm(), WithinAbs(0, DELTA_DISTANCE)); // this should always be the case
    CHECK_THAT((reference.local_from_global_pos(Pos{-1, 0, 0}) - Pos(0, 1, 0)).norm(), WithinAbs(0, DELTA_DISTANCE));
    CHECK_THAT((reference.local_from_global_pos(Pos{0, -1, 0}) - Pos(-sqrt2_2, 0, -sqrt2_2)).norm(), WithinAbs(0, DELTA_DISTANCE));
    CHECK_THAT((reference.local_from_global_pos(Pos{0, 0, -1}) - Pos(sqrt2_2, 0, -sqrt2_2)).norm(), WithinAbs(0, DELTA_DISTANCE));
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
    CHECK_THAT((ref3.global_from_local_pos(Pos{1, 2, 3}) - Pos(2, 3, 4)).norm(), WithinAbs(0, DELTA_DISTANCE));
    CHECK_THAT((ref3.global_from_local_pos(Pos{-1, -1, -1}) - POS_ZERO).norm(), WithinAbs(0, DELTA_DISTANCE));
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
    CHECK_THAT((ref3.global_from_local_pos(Pos{1, 2, 3}) - Pos(2, 3, 4)).norm(), WithinAbs(0, DELTA_DISTANCE));
    CHECK_THAT((ref3.global_from_local_pos(Pos{-1, -1, -1}) - POS_ZERO).norm(), WithinAbs(0, DELTA_DISTANCE));
}
