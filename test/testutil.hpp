//
// Created by Tristan Krause on 2026-05-29.
//

#pragma once

#include "types.hpp"
#include "reference.hpp"

static constexpr double TEST_MARGIN = 1e-6;

template <typename T>
bool isclose(T a, T b)
{
    return nc::isclose(nc::NdArray<T>{a}, nc::NdArray<T>{b})[0];
}

void require_close_position(Vec3 const &actual, Vec3 const &expected);

void require_close_array(NdArray const& actual, NdArray const& expected);

void test_inverse_transformation(Reference const &reference, Vec3 const &pos);

void test_basic_transformations(Reference const &reference);
