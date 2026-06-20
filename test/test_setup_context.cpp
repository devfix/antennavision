//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "setup.hpp"

TEST_CASE("setup context only variables", "[TestSetupContext]")
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

TEST_CASE("setup context with references", "[TestSetupContext]")
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
      "translation": {
        "x": "x",
        "y": "y",
        "z": "z"
      },
      "rotation": {
        "yaw": "yaw",
        "pitch": "pitch",
        "roll": "roll"
      }
    }
  ]
}
)JSON");
    auto const su = Setup::from_json(js);
    REQUIRE(su->get_reference_by_id("ref1").pos.x == Catch::Approx(1.0));
    REQUIRE(su->get_reference_by_id("ref1").pos.y == Catch::Approx(2.0));
    REQUIRE(su->get_reference_by_id("ref1").pos.z == Catch::Approx(3.0));
    REQUIRE(su->get_reference_by_id("ref1").rotation.yaw() == Catch::Approx(0.1 * pi));
    REQUIRE(su->get_reference_by_id("ref1").rotation.pitch() == Catch::Approx(0.2 * pi));
    REQUIRE(su->get_reference_by_id("ref1").rotation.roll() == Catch::Approx(0.3 * pi));
}
