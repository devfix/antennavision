//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <nlohmann/json.hpp>
#include "setup/setup.hpp"
#include "testutil.hpp"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

TEST_CASE("setup without rotation", "[Setup]")
{
    ojson const js = ojson::parse(R"(
{
  "metadata": {
    "setup_name": "test_setup_without_rotation",
    "version": "1.0.1"
  },
  "references": [
    {
      "id": "ref1",
      "origin": "",
      "pos": [1, 0, 0]
    },
    {
      "id": "ref2",
      "origin": "ref1",
      "pos": [0, 1, 0]
    },
    {
      "id": "ref3",
      "origin": "ref2",
      "pos": [0, 0, 1]
    }
  ]
}
)");
    auto su = setup::Setup(js);
    auto const& ref1 = su.get_reference("ref1");
    auto const& ref2 = su.get_reference("ref2");
    auto const& ref3 = su.get_reference("ref3");
    test_basic_transformations(ref1);
    test_basic_transformations(ref2);
    test_basic_transformations(ref3);
    CHECK_THAT((ref3.global_from_local_pos(Pos{1, 2, 3}) - Pos(2, 3, 4)).norm(), WithinAbs(0, DELTA_DISTANCE));
    CHECK_THAT((ref3.global_from_local_pos(Pos{-1, -1, -1}) - POS_ZERO).norm(), WithinAbs(0, DELTA_DISTANCE));
}

TEST_CASE("setup with rotation", "[Setup]")
{
    ojson const js = ojson::parse(R"(
{
  "metadata": {
    "setup_name": "test_setup_with_rotation",
    "version": "1.0.1"
  },
  "references": [
    {
      "id": "ref1",
      "origin": "",
      "pos": [1, 0, 0],
      "rot": { "yaw": "0.5*pi", "pitch": 0, "roll": 0 }
    },
    {
      "id": "ref2",
      "origin": "ref1",
      "pos": [1, 0, 0],
      "rot": { "yaw": 0, "pitch": "-0.5*pi", "roll": 0 }
    },
    {
      "id": "ref3",
      "origin": "ref2",
      "pos": [1, 0, 0],
      "rot": { "yaw": "-0.5*pi", "pitch": 0, "roll": "-0.5*pi" }
    }
  ]
}
)");
    setup::Setup su(js);
    auto const& ref1 = su.get_reference("ref1");
    auto const& ref2 = su.get_reference("ref2");
    auto const& ref3 = su.get_reference("ref3");
    test_basic_transformations(ref1);
    test_basic_transformations(ref2);
    test_basic_transformations(ref3);
    CHECK_THAT((ref3.global_from_local_pos(Pos{1, 2, 3}) - Pos(2, 3, 4)).norm(), WithinAbs(0, DELTA_DISTANCE));
    CHECK_THAT((ref3.global_from_local_pos(Pos{-1, -1, -1}) - POS_ZERO).norm(), WithinAbs(0, DELTA_DISTANCE));
}

TEST_CASE("setup context only variables", "[Setup]")
{
    auto const js = nlohmann::ordered_json::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test_setup_context",
    "version": "1.0.1"
  },
  "variables": {
    "x": 2.0,
    "y": "x * 3.0",
    "z": "x + y",
    "phi": "2 * pi",
    "a": "1 + sin(phi)",
    "b": "3 * cos(phi)",
    "c": "sin(phi - pi/2)"
  }
}
)JSON");
    setup::Setup const su(js);
    CHECK_THAT(su.get_double("x"), WithinAbs(2.0, DELTA_DISTANCE));
    CHECK_THAT(su.get_double("y"), WithinAbs(6.0, DELTA_DISTANCE));
    CHECK_THAT(su.get_double("z"), WithinAbs(8.0, DELTA_DISTANCE));
    CHECK_THAT(su.get_double("phi"), WithinAbs(2 * pi, DELTA_PHASE));
    CHECK_THAT(su.get_double("a"), WithinAbs(1.0, DELTA_DISTANCE));
    CHECK_THAT(su.get_double("b"), WithinAbs(3.0, DELTA_DISTANCE));
    CHECK_THAT(su.get_double("c"), WithinAbs(-1.0, DELTA_DISTANCE));

    // check mathematical and physical constants
    CHECK_THAT(su.get_double("c0"), WithinRel(c0, 1e-12));
    CHECK_THAT(su.get_double("mu0"), WithinRel(mu0, 1e-12));
    CHECK_THAT(su.get_double("epsilon0"), WithinRel(epsilon0, 1e-12));
    CHECK_THAT(su.get_double("Z0"), WithinRel(Z0, 1e-12));
}

TEST_CASE("setup context with references", "[Setup]")
{
    auto const js = nlohmann::ordered_json::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test_setup_context",
    "version": "1.0.1"
  },
  "variables": {
    "x": 1.0,
    "y": 2.0,
    "z": 3.0,
    "yaw": "0.1*pi",
    "pitch": "0.2*pi",
    "roll": "0.3*pi"
  },
  "references": [
    {
      "id": "ref1",
      "origin": "",
      "pos": ["x", "y", "z"],
      "rot": { "yaw": "yaw", "pitch": "pitch", "roll": "roll" }
    }
  ]
}
)JSON");
    setup::Setup su(js);
    CHECK_THAT(su.get_reference("ref1").pos.x, WithinAbs(1.0, DELTA_DISTANCE));
    CHECK_THAT(su.get_reference("ref1").pos.y, WithinAbs(2.0, DELTA_DISTANCE));
    CHECK_THAT(su.get_reference("ref1").pos.z, WithinAbs(3.0, DELTA_DISTANCE));
    CHECK_THAT(su.get_reference("ref1").rot.yaw(), WithinAbs(0.1 * pi, DELTA_PHASE));
    CHECK_THAT(su.get_reference("ref1").rot.pitch(), WithinAbs(0.2 * pi, DELTA_PHASE));
    CHECK_THAT(su.get_reference("ref1").rot.roll(), WithinAbs(0.3 * pi, DELTA_DISTANCE));
}
