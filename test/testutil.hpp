//
// Created by Tristan Krause on 2026-05-29.
//

#pragma once

#include <NumCpp/Functions/isclose.hpp>
#include "types/math.hpp"
#include "reference.hpp"

static constexpr double TEST_MARGIN = 1e-6;

template <typename T>
bool isclose(T a, T b)
{ return nc::isclose(nc::NdArray<T>{a}, nc::NdArray<T>{b})[0]; }

#define REQUIRE_CLOSE_POSITION(actual, expected)                                                                                                               \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        REQUIRE((actual).toNdArray().at(0) == Catch::Approx((expected).toNdArray().at(0)).margin(TEST_MARGIN));                                                \
        REQUIRE((actual).toNdArray().at(1) == Catch::Approx((expected).toNdArray().at(1)).margin(TEST_MARGIN));                                                \
        REQUIRE((actual).toNdArray().at(2) == Catch::Approx((expected).toNdArray().at(2)).margin(TEST_MARGIN));                                                \
    }                                                                                                                                                          \
    while (0)

#define REQUIRE_CLOSE_ARRAY(actual, expected)                                                                                                                  \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        auto const& actual_ = (actual);                                                                                                                        \
        auto const& expected_ = (expected);                                                                                                                    \
        REQUIRE(actual_.shape() == expected_.shape());                                                                                                         \
        for (nc::uint32 r = 0; r < expected_.shape().rows; r++)                                                                                                \
        {                                                                                                                                                      \
            for (nc::uint32 c = 0; c < expected_.shape().cols; c++) { REQUIRE(actual_(r, c) == Catch::Approx(expected_(r, c)).margin(TEST_MARGIN)); }          \
        }                                                                                                                                                      \
    }                                                                                                                                                          \
    while (0)

void test_inverse_transformation(reference::Reference const& reference, Pos const& pos);

void test_basic_transformations(reference::Reference const& reference);
