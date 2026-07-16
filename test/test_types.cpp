//
// Created by Tristan Krause on 2026-07-16.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "types.hpp"


TEST_CASE("serialize to JSON", "[types][complex_t]")
{
    double constexpr real = 1.23;
    double constexpr imag = 4.56;
    complex_t constexpr c(real, imag);

    SECTION("Serializing using nlohmann::json") {
        json js = c;

        REQUIRE(js.is_array());
        REQUIRE(js.size() == 2);
        REQUIRE(js[0].get<double>() == real);
        REQUIRE(js[1].get<double>() == imag);
    }

    SECTION("Serializing using nlohmann::ordered_json") {
        ojson oj = c;

        REQUIRE(oj.is_array());
        REQUIRE(oj.size() == 2);
        REQUIRE(oj[0].get<double>() == real);
        REQUIRE(oj[1].get<double>() == imag);
    }
}

TEST_CASE("deserialize from JSON", "[types][complex_t]") {
    double constexpr real = 1.23;
    double constexpr imag = 4.56;
    SECTION("Deserializing valid standard JSON array") {
        json const js = json::array({real, imag});
        auto const c = js.get<std::complex<double>>();
        REQUIRE(c.real() == real);
        REQUIRE(c.imag() == imag);
    }

    SECTION("Deserializing valid ordered JSON array") {
        ojson oj = ojson::array({real, imag});
        auto const c = oj.get<std::complex<double>>();
        REQUIRE(c.real() == real);
        REQUIRE(c.imag() == imag);
    }

    SECTION("Deserializing invalid JSON structure triggers error") {
        // Test 1: Too many elements
        json bad_size = json::array({1.0, 2.0, 3.0});
        REQUIRE_THROWS_AS(bad_size.get<std::complex<double>>(), json::type_error);

        // Test 2: Wrong types
        json bad_types = json::array({"real", "imag"});
        REQUIRE_THROWS(bad_types.get<std::complex<double>>());

        // Test 3: Object instead of Array
        json bad_format = json::object({{"real", 1.0}, {"imag", 2.0}});
        REQUIRE_THROWS_AS(bad_format.get<std::complex<double>>(), json::type_error);
    }
}

TEST_CASE("std::vector<double> (de)serialization", "[types][vector]") {
    std::vector<double> vec{1.1, 2.2, 3.3};

    SECTION("Serialization to json and ordered_json") {
        json js = vec;
        REQUIRE(js.is_array());
        REQUIRE(js.size() == 3);
        REQUIRE(js[0].get<double>() == 1.1);

        ojson oj = vec;
        REQUIRE(oj.is_array());
        REQUIRE(oj.size() == 3);
        REQUIRE(oj[2].get<double>() == 3.3);
    }

    SECTION("Deserialization from valid JSON array") {
        json js = json::array({4.4, 5.5});
        auto parsed = js.get<std::vector<double>>();
        
        REQUIRE(parsed.size() == 2);
        REQUIRE(parsed[0] == 4.4);
        REQUIRE(parsed[1] == 5.5);
    }

    SECTION("Deserialization error handling") {
        // Expected an array, got an integer
        json bad_type = 42;
        REQUIRE_THROWS_AS(bad_type.get<std::vector<double>>(), json::type_error);

        // Expected an array of doubles, got an array containing a string
        json bad_element = json::array({1.1, "not-a-double", 3.3});
        REQUIRE_THROWS(bad_element.get<std::vector<double>>());
    }
}

TEST_CASE("std::vector<std::complex<double>> nested (de)serialization", "[types][vector]") {
    std::vector<complex_t> complex_vec{
        complex_t{1.0, -1.0},
        complex_t{0.0, 2.5}
    };

    SECTION("Composed serialization") {
        json js = complex_vec;
        
        REQUIRE(js.is_array());
        REQUIRE(js.size() == 2);
        
        // Inside the vector array, we should have 2-element arrays for the complex numbers
        REQUIRE(js[0].is_array());
        REQUIRE(js[0].size() == 2);
        REQUIRE(js[0][0].get<double>() == 1.0);
        REQUIRE(js[0][1].get<double>() == -1.0);

        REQUIRE(js[1][0].get<double>() == 0.0);
        REQUIRE(js[1][1].get<double>() == 2.5);
    }

    SECTION("Composed deserialization") {
        json js = json::parse(R"(
            [
                [5.0, 12.0],
                [-3.0, -4.0]
            ]
        )");

        auto parsed = js.get<std::vector<complex_t>>();
        
        REQUIRE(parsed.size() == 2);
        REQUIRE(parsed[0].real() == 5.0);
        REQUIRE(parsed[0].imag() == 12.0);
        REQUIRE(parsed[1].real() == -3.0);
        REQUIRE(parsed[1].imag() == -4.0);
    }

    SECTION("Nested deserialization error handling") {
        // Vector is an array, but one of the complex elements is not a 2-element array
        json bad_nested = json::parse(R"(
            [
                [1.0, 2.0],
                [3.0] 
            ]
        )");
        // The nested std::complex deserializer should throw a validation exception
        REQUIRE_THROWS_AS(bad_nested.get<std::vector<complex_t>>(), json::type_error);
    }
}

TEST_CASE("nc::Vec2 Serialization", "[types][vec2]") {
    nc::Vec2 vec{3.5, -7.2};

    SECTION("Serializing to standard nlohmann::json") {
        json js = vec;
        
        REQUIRE(js.is_array());
        REQUIRE(js.size() == 2);
        REQUIRE(js[0].get<double>() == 3.5);
        REQUIRE(js[1].get<double>() == -7.2);
    }

    SECTION("Serializing to nlohmann::ordered_json") {
        ojson oj = vec;
        
        REQUIRE(oj.is_array());
        REQUIRE(oj.size() == 2);
        REQUIRE(oj[0].get<double>() == 3.5);
        REQUIRE(oj[1].get<double>() == -7.2);
    }
}

TEST_CASE("nc::Vec2 Deserialization", "[types][vec2]") {
    SECTION("Deserializing valid standard JSON array") {
        json js = json::array({12.0, 5.5});
        auto vec = js.get<nc::Vec2>();

        REQUIRE(vec.x == 12.0);
        REQUIRE(vec.y == 5.5);
    }

    SECTION("Deserializing valid ordered JSON array") {
        ojson oj = ojson::array({-1.0, 0.0});
        auto vec = oj.get<nc::Vec2>();

        REQUIRE(vec.x == -1.0);
        REQUIRE(vec.y == 0.0);
    }

    SECTION("Deserializing invalid JSON structure triggers error") {
        // Test 1: Too many elements (expected exactly 2)
        json bad_size = json::array({1.0, 2.0, 3.0});
        REQUIRE_THROWS_AS(bad_size.get<nc::Vec2>(), json::type_error);

        // Test 2: Too few elements
        json too_short = json::array({1.0});
        REQUIRE_THROWS_AS(too_short.get<nc::Vec2>(), json::type_error);

        // Test 3: Wrong types (strings instead of doubles)
        json bad_types = json::array({"x_axis", "y_axis"});
        REQUIRE_THROWS(bad_types.get<nc::Vec2>());

        // Test 4: Object format instead of Array
        json bad_format = json::object({{"x", 3.5}, {"y", -7.2}});
        REQUIRE_THROWS_AS(bad_format.get<nc::Vec2>(), json::type_error);
    }
}

TEST_CASE("nc::Vec3 Serialization", "[types][vec3]") {
    nc::Vec3 vec{1.0, -2.5, 3.75};

    SECTION("Serializing to standard nlohmann::json") {
        json js = vec;

        REQUIRE(js.is_array());
        REQUIRE(js.size() == 3);
        REQUIRE(js[0].get<double>() == 1.0);
        REQUIRE(js[1].get<double>() == -2.5);
        REQUIRE(js[2].get<double>() == 3.75);
    }

    SECTION("Serializing to nlohmann::ordered_json") {
        ojson oj = vec;

        REQUIRE(oj.is_array());
        REQUIRE(oj.size() == 3);
        REQUIRE(oj[0].get<double>() == 1.0);
        REQUIRE(oj[1].get<double>() == -2.5);
        REQUIRE(oj[2].get<double>() == 3.75);
    }
}

TEST_CASE("nc::Vec3 Deserialization", "[types][vec3]") {
    SECTION("Deserializing valid standard JSON array") {
        json js = json::array({10.0, 20.0, 30.0});
        auto vec = js.get<nc::Vec3>();

        REQUIRE(vec.x == 10.0);
        REQUIRE(vec.y == 20.0);
        REQUIRE(vec.z == 30.0);
    }

    SECTION("Deserializing valid ordered JSON array") {
        ojson oj = ojson::array({-1.5, 0.0, 1.5});
        auto vec = oj.get<nc::Vec3>();

        REQUIRE(vec.x == -1.5);
        REQUIRE(vec.y == 0.0);
        REQUIRE(vec.z == 1.5);
    }

    SECTION("Deserializing invalid JSON structure triggers error") {
        // Test 1: Too many elements (expected exactly 3)
        json bad_size_large = json::array({1.0, 2.0, 3.0, 4.0});
        REQUIRE_THROWS_AS(bad_size_large.get<nc::Vec3>(), json::type_error);

        // Test 2: Too few elements
        json bad_size_small = json::array({1.0, 2.0});
        REQUIRE_THROWS_AS(bad_size_small.get<nc::Vec3>(), json::type_error);

        // Test 3: Wrong types (contains string)
        json bad_types = json::array({1.0, "middle", 3.0});
        REQUIRE_THROWS(bad_types.get<nc::Vec3>());

        // Test 4: Object format instead of Array
        json bad_format = json::object({{"x", 1.0}, {"y", -2.5}, {"z", 3.75}});
        REQUIRE_THROWS_AS(bad_format.get<nc::Vec3>(), json::type_error);
    }
}
