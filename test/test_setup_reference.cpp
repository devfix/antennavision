//
// Created by Tristan Krause on 2026-05-26.
//

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "../include/setup.hpp"
#include "testutil.hpp"

TEST_CASE("setup without rotation", "[TestSetupReferences]")
{
    json const js = json::parse(R"(
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
    su->export_to_three();
    auto const& ref1 = su->get_reference_by_id("ref1");
    auto const& ref2 = su->get_reference_by_id("ref2");
    auto const& ref3 = su->get_reference_by_id("ref3");
    test_basic_transformations(ref1);
    test_basic_transformations(ref2);
    test_basic_transformations(ref3);
    require_close_position(ref3.global_from_local(Vec3{1, 2, 3}), Vec3(2, 3, 4));
    require_close_position(ref3.global_from_local(Vec3{-1, -1, -1}), POS_ZERO);
}

TEST_CASE("setup with rotation", "[TestSetupReferences]")
{
    json const js = json::parse(R"(
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
    su->export_to_three();
    auto const &ref1 = su->get_reference_by_id("ref1");
    auto const &ref2 = su->get_reference_by_id("ref2");
    auto const &ref3 = su->get_reference_by_id("ref3");
    test_basic_transformations(ref1);
    test_basic_transformations(ref2);
    test_basic_transformations(ref3);
    require_close_position(ref3.global_from_local(Vec3{1, 2, 3}), Vec3(2, 3, 4));
    require_close_position(ref3.global_from_local(Vec3{-1, -1, -1}), POS_ZERO);
}
