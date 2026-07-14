//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "../include/setup.hpp"
#include "testutil.hpp"

TEST_CASE("setup without rotation", "[Setup]")
{
    ojson const js = ojson::parse(R"(
{
  "metadata": {
    "setup_name": "test_setup_without_rotation"
  },
  "references": [
    {
      "id": "ref1",
      "origin": "",
      "pos": {
        "x": 1,
        "y": 0,
        "z": 0
      },
      "rot": {
        "yaw": 0.0,
        "pitch": 0.0,
        "roll": 0.0
      }
    },
    {
      "id": "ref2",
      "origin": "ref1",
      "pos": {
        "x": 0,
        "y": 1,
        "z": 0
      },
      "rot": {
        "yaw": 0.0,
        "pitch": 0.0,
        "roll": 0.0
      }
    },
    {
      "id": "ref3",
      "origin": "ref2",
      "pos": {
        "x": 0,
        "y": 0,
        "z": 1
      },
      "rot": {
        "yaw": 0.0,
        "pitch": 0.0,
        "roll": 0.0
      }
    }
  ]
}
)");
    auto const su = Setup::from_json(js);
    auto const& ref1 = su->get_reference("ref1");
    auto const& ref2 = su->get_reference("ref2");
    auto const& ref3 = su->get_reference("ref3");
    test_basic_transformations(ref1);
    test_basic_transformations(ref2);
    test_basic_transformations(ref3);
    REQUIRE_CLOSE_POSITION(ref3.global_from_local_pos(pos_t{1, 2, 3}), pos_t(2, 3, 4));
    REQUIRE_CLOSE_POSITION(ref3.global_from_local_pos(pos_t{-1, -1, -1}), POS_ZERO);
}

TEST_CASE("setup with rotation", "[Setup]")
{
    ojson const js = ojson::parse(R"(
{
  "metadata": {
    "setup_name": "test_setup_with_rotation"
  },
  "references": [
    {
      "id": "ref1",
      "origin": "",
      "pos": {
        "x": 1,
        "y": 0,
        "z": 0
      },
      "rot": {
        "yaw": 0.5,
        "pitch": 0.0,
        "roll": 0.0
      }
    },
    {
      "id": "ref2",
      "origin": "ref1",
      "pos": {
        "x": 1,
        "y": 0,
        "z": 0
      },
      "rot": {
        "yaw": 0.0,
        "pitch": -0.5,
        "roll": 0.0
      }
    },
    {
      "id": "ref3",
      "origin": "ref2",
      "pos": {
        "x": 1,
        "y": 0,
        "z": 0
      },
      "rot": {
        "yaw": -0.5,
        "pitch": 0.0,
        "roll": -0.5
      }
    }
  ]
}
)");
    auto const su = Setup::from_json(js);
    auto const &ref1 = su->get_reference("ref1");
    auto const &ref2 = su->get_reference("ref2");
    auto const &ref3 = su->get_reference("ref3");
    test_basic_transformations(ref1);
    test_basic_transformations(ref2);
    test_basic_transformations(ref3);
    REQUIRE_CLOSE_POSITION(ref3.global_from_local_pos(pos_t{1, 2, 3}), pos_t(2, 3, 4));
    REQUIRE_CLOSE_POSITION(ref3.global_from_local_pos(pos_t{-1, -1, -1}), POS_ZERO);
}

TEST_CASE("setup context only variables", "[Setup]")
{
    auto const js = nlohmann::ordered_json::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test_setup_context"
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
    auto const su = Setup::from_json(js);
    REQUIRE(su->variables.at("x") == Catch::Approx(2.0));
    REQUIRE(su->variables.at("y") == Catch::Approx(6.0));
    REQUIRE(su->variables.at("z") == Catch::Approx(8.0));
    REQUIRE(su->variables.at("phi") == Catch::Approx(2 * pi));
    REQUIRE(su->variables.at("a") == Catch::Approx(1.0));
    REQUIRE(su->variables.at("b") == Catch::Approx(3.0));
    REQUIRE(su->variables.at("c") == Catch::Approx(-1.0));
}

TEST_CASE("setup context with references", "[Setup]")
{
    auto const js = nlohmann::ordered_json::parse(R"JSON(
{
  "metadata": {
    "setup_name": "test_setup_context"
  },
  "variables": {
    "x": 1.0,
    "y": 2.0,
    "z": 3.0,
    "yaw": 0.1,
    "pitch": 0.2,
    "roll": 0.3
  },
  "references": [
    {
      "id": "ref1",
      "origin": "",
      "pos": {
        "x": "x",
        "y": "y",
        "z": "z"
      },
      "rot": {
        "yaw": "yaw",
        "pitch": "pitch",
        "roll": "roll"
      }
    }
  ]
}
)JSON");
    auto const su = Setup::from_json(js);
    REQUIRE(su->get_reference("ref1").pos.x == Catch::Approx(1.0));
    REQUIRE(su->get_reference("ref1").pos.y == Catch::Approx(2.0));
    REQUIRE(su->get_reference("ref1").pos.z == Catch::Approx(3.0));
    REQUIRE(su->get_reference("ref1").rotation.yaw() == Catch::Approx(0.1 * pi));
    REQUIRE(su->get_reference("ref1").rotation.pitch() == Catch::Approx(0.2 * pi));
    REQUIRE(su->get_reference("ref1").rotation.roll() == Catch::Approx(0.3 * pi));
}
