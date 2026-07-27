//
// Created by Tristan Krause on 2026-07-24.
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <nlohmann/json.hpp>
#include <vector>

#include "setup/sweep.hpp"

using Catch::Matchers::WithinAbs;

TEST_CASE("ListSweep Properties and JSON Serialization", "[sweep][list][json]")
{
    SECTION("Constructor sorts input values and correctly reports boundaries")
    {
        std::vector<double> const unsorted_vals = {5.0, 1.0, 3.0, 2.0, 4.0};
        sweep::ListSweep const sweep{"list_01", unsorted_vals};

        CHECK(sweep.id() == "list_01");
        CHECK(sweep.size() == 5);
        CHECK_THAT(sweep.begin_val(), WithinAbs(1.0, 1e-9));
        CHECK_THAT(sweep.end_val(), WithinAbs(5.0, 1e-9));

        auto const vals = sweep.values();
        CHECK_THAT(vals[0], WithinAbs(1.0, 1e-9));
        CHECK_THAT(vals[1], WithinAbs(2.0, 1e-9));
        CHECK_THAT(vals[2], WithinAbs(3.0, 1e-9));
        CHECK_THAT(vals[3], WithinAbs(4.0, 1e-9));
        CHECK_THAT(vals[4], WithinAbs(5.0, 1e-9));
    }

    SECTION("Round-trip JSON serialization preserves id and values")
    {
        std::vector<double> const input_vals = {10.0, 20.0, 30.0};
        sweep::ListSweep const sweep{"list_02", input_vals};

        nlohmann::json const js = sweep;
        CHECK(js.at("id") == sweep.id());
        CHECK(js.at("values") == sweep.values());

        auto const deserialized = js.get<sweep::ListSweep>();
        CHECK(deserialized.id() == sweep.id());
        CHECK(deserialized.size() == sweep.size());
        CHECK_THAT(deserialized.begin_val(), WithinAbs(10.0, 1e-9));
        CHECK_THAT(deserialized.end_val(), WithinAbs(30.0, 1e-9));
    }

    SECTION("Deserialization throws on missing required structure")
    {
        nlohmann::json const js_invalid = {
            {"id", "list_invalid"}
            // Missing "values" array
        };

        REQUIRE_THROWS(js_invalid.get<sweep::ListSweep>());
    }
}

TEST_CASE("LinearSweep Properties and JSON Serialization", "[sweep][linear][json]")
{
    SECTION("Linear interpolation generates evenly spaced values")
    {
        sweep::LinearSweep const sweep{"lin_01", 0.0, 10.0, 5};

        CHECK(sweep.id() == "lin_01");
        CHECK(sweep.size() == 5);
        CHECK_THAT(sweep.begin_val(), WithinAbs(0.0, 1e-9));
        CHECK_THAT(sweep.end_val(), WithinAbs(10.0, 1e-9));

        auto const vals = sweep.values();
        REQUIRE(vals.size() == 5);
        CHECK_THAT(vals[0], WithinAbs(0.0, 1e-9));
        CHECK_THAT(vals[1], WithinAbs(2.5, 1e-9));
        CHECK_THAT(vals[2], WithinAbs(5.0, 1e-9));
        CHECK_THAT(vals[3], WithinAbs(7.5, 1e-9));
        CHECK_THAT(vals[4], WithinAbs(10.0, 1e-9));
    }

    SECTION("Round-trip JSON serialization preserves properties")
    {
        sweep::LinearSweep const sweep{"lin_02", 1.0, 5.0, 3};

        nlohmann::json const js = sweep;
        CHECK(js.at("id") == sweep.id());
        CHECK_THAT(js.at("begin").get<double>(), WithinAbs(1.0, 1e-9));
        CHECK_THAT(js.at("end").get<double>(), WithinAbs(5.0, 1e-9));
        CHECK(js.at("size") == 3);

        auto const deserialized = js.get<sweep::LinearSweep>();
        CHECK(deserialized.id() == sweep.id());
        CHECK(deserialized.size() == sweep.size());
        CHECK_THAT(deserialized.begin_val(), WithinAbs(1.0, 1e-9));
        CHECK_THAT(deserialized.end_val(), WithinAbs(5.0, 1e-9));
    }

    SECTION("Deserialization ignores dummy values array and recomputes from bounds")
    {
        nlohmann::json const js = {
            {"id", "lin_03"},
            {"begin", 0.0},
            {"end", 100.0},
            {"size", 3},
            {"values", {999.0, 999.0, 999.0}} // Should be ignored by from_json
        };

        auto const sweep = js.get<sweep::LinearSweep>();
        auto const vals = sweep.values();

        CHECK_THAT(vals[0], WithinAbs(0.0, 1e-9));
        CHECK_THAT(vals[1], WithinAbs(50.0, 1e-9));
        CHECK_THAT(vals[2], WithinAbs(100.0, 1e-9));
    }
}

TEST_CASE("LogSweep Properties and JSON Serialization", "[sweep][log][json]")
{
    SECTION("Logarithmic interpolation generates expected curve")
    {
        sweep::LogSweep const sweep{"log_01", 1.0, 10.0, 3, 10.0};

        CHECK(sweep.id() == "log_01");
        CHECK(sweep.size() == 3);
        CHECK_THAT(sweep.base(), WithinAbs(10.0, 1e-9));
        CHECK_THAT(sweep.begin_val(), WithinAbs(1.0, 1e-9));
        CHECK_THAT(sweep.end_val(), WithinAbs(10.0, 1e-9));

        auto const vals = sweep.values();
        REQUIRE(vals.size() == 3);
        CHECK_THAT(vals[0], WithinAbs(1.0, 1e-9));
        CHECK_THAT(vals[2], WithinAbs(10.0, 1e-9));
        // Middle value u = (10^(0.5) - 1) / (10 - 1) = (3.16227766 - 1) / 9 = ~0.240253
        // val = 1.0 + 0.240253 * 9.0 = 3.16227766...
        CHECK_THAT(vals[1], WithinAbs(std::sqrt(10.0), 1e-6));
    }

    SECTION("Deserialization defaults base to 10.0 when omitted from JSON")
    {
        nlohmann::json const js = {
            {"id", "log_02"},
            {"begin", 1.0},
            {"end", 100.0},
            {"size", 5}
            // "base" field explicitly omitted
        };

        auto const sweep = js.get<sweep::LogSweep>();
        CHECK_THAT(sweep.base(), WithinAbs(10.0, 1e-9));
    }

    SECTION("Round-trip JSON serialization preserves custom base")
    {
        sweep::LogSweep const sweep{"log_03", 2.0, 16.0, 4, 2.0};

        nlohmann::json const js = sweep;
        CHECK_THAT(js.at("base").get<double>(), WithinAbs(2.0, 1e-9));

        auto const deserialized = js.get<sweep::LogSweep>();
        CHECK_THAT(deserialized.base(), WithinAbs(2.0, 1e-9));
        CHECK_THAT(deserialized.begin_val(), WithinAbs(2.0, 1e-9));
        CHECK_THAT(deserialized.end_val(), WithinAbs(16.0, 1e-9));
    }
}

TEST_CASE("Sweep Variant Helpers", "[sweep][variant]")
{
    SECTION("Helper functions dispatch correctly to underlying LinearSweep variant")
    {
        sweep::Sweep const sweep = sweep::LinearSweep{"variant_lin", 0.0, 20.0, 3};

        CHECK(sweep::get_id(sweep) == "variant_lin");
        CHECK(sweep::get_size(sweep) == 3);
        CHECK_THAT(sweep::get_begin_val(sweep), WithinAbs(0.0, 1e-9));
        CHECK_THAT(sweep::get_end_val(sweep), WithinAbs(20.0, 1e-9));

        auto const vals = sweep::get_values(sweep);
        REQUIRE(vals.size() == 3);
        CHECK_THAT(vals[1], WithinAbs(10.0, 1e-9));
    }

    SECTION("Helper functions dispatch correctly to underlying ListSweep variant")
    {
        std::vector<double> const input_vals = {100.0, 50.0};
        sweep::Sweep const sweep = sweep::ListSweep{"variant_list", input_vals};

        CHECK(sweep::get_id(sweep) == "variant_list");
        CHECK(sweep::get_size(sweep) == 2);
        CHECK_THAT(sweep::get_begin_val(sweep), WithinAbs(50.0, 1e-9)); // Sorted
        CHECK_THAT(sweep::get_end_val(sweep), WithinAbs(100.0, 1e-9));
    }
}