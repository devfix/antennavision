//
// Created by Tristan Krause on 2026-05-29.
//

#include "testutil.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

void require_close_position(Vec3 const &actual, Vec3 const &expected)
{
    REQUIRE(actual.toNdArray().at(0) == Catch::Approx(expected.toNdArray().at(0)).margin(TEST_MARGIN));
    REQUIRE(actual.toNdArray().at(1) == Catch::Approx(expected.toNdArray().at(1)).margin(TEST_MARGIN));
    REQUIRE(actual.toNdArray().at(2) == Catch::Approx(expected.toNdArray().at(2)).margin(TEST_MARGIN));
}

void require_close_array(NdArray const &actual, NdArray const &expected)
{
    REQUIRE(actual.shape() == expected.shape());
    for (nc::uint32 r = 0; r < expected.shape().rows; r++)
    {
        for (nc::uint32 c = 0; c < expected.shape().cols; c++) { REQUIRE(actual(r, c) == Catch::Approx(expected(r, c)).margin(TEST_MARGIN)); }
    }
}

void test_inverse_transformation(Reference const &reference, Vec3 const &pos)
{
    require_close_position(reference.local_from_global(reference.global_from_local(pos)), pos);
    require_close_position(reference.global_from_local(reference.local_from_global(pos)), pos);
}

void test_basic_transformations(Reference const &reference)
{
    test_inverse_transformation(reference, POS_ZERO);
    test_inverse_transformation(reference, reference.pos);
    if (reference.origin)
    {
        require_close_position(reference.local_from_global(reference.origin->global_from_local(reference.pos)), POS_ZERO);
        require_close_position(reference.global_from_local(POS_ZERO), reference.origin->global_from_local(reference.pos));
    }
    else
    {
        require_close_position(reference.local_from_global(reference.pos), POS_ZERO);
        require_close_position(reference.global_from_local(POS_ZERO), reference.pos);
    }
}
