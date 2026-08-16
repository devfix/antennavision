//
// Created by Tristan Krause on 2026-05-29.
//

#pragma once

#include <NumCpp/Functions/isclose.hpp>
#include "reference.hpp"
#include "types/math.hpp"

static double constexpr EPSILON_MAG = 1e-4;
static double constexpr DELTA_MAG = 1e-4;
static double constexpr DELTA_PHASE = 1e-3;
static double constexpr DELTA_DISTANCE = 1e-4;

template <typename T>
bool isclose(T a, T b)
{ return nc::isclose(nc::NdArray<T>{a}, nc::NdArray<T>{b})[0]; }

#define CHECK_CLOSE_POSITION(actual, expected)                                                                                                                 \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        REQUIRE((actual).toNdArray().at(0) == Catch::Approx((expected).toNdArray().at(0)).margin(DELTA_DISTANCE));                                             \
        REQUIRE((actual).toNdArray().at(1) == Catch::Approx((expected).toNdArray().at(1)).margin(DELTA_DISTANCE));                                             \
        REQUIRE((actual).toNdArray().at(2) == Catch::Approx((expected).toNdArray().at(2)).margin(DELTA_DISTANCE));                                             \
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
            for (nc::uint32 c = 0; c < expected_.shape().cols; c++) { REQUIRE(actual_(r, c) == Catch::Approx(expected_(r, c)).margin(DELTA_DISTANCE)); }       \
        }                                                                                                                                                      \
    }                                                                                                                                                          \
    while (0)

void test_inverse_transformation(reference::Reference const& reference, Pos const& pos);

void test_basic_transformations(reference::Reference const& reference);
