//
// Created by Tristan Krause on 2026-05-29.
//

#include "testutil.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

void test_inverse_transformation(Reference const &reference, pos_t const &pos)
{
    REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.global_from_local_pos(pos)), pos);
    REQUIRE_CLOSE_POSITION(reference.global_from_local_pos(reference.local_from_global_pos(pos)), pos);
}

void test_basic_transformations(Reference const &reference)
{
    test_inverse_transformation(reference, POS_ZERO);
    test_inverse_transformation(reference, reference.pos);
    if (reference.origin)
    {
        REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.origin->global_from_local_pos(reference.pos)), POS_ZERO);
        REQUIRE_CLOSE_POSITION(reference.global_from_local_pos(POS_ZERO), reference.origin->global_from_local_pos(reference.pos));
    }
    else
    {
        REQUIRE_CLOSE_POSITION(reference.local_from_global_pos(reference.pos), POS_ZERO);
        REQUIRE_CLOSE_POSITION(reference.global_from_local_pos(POS_ZERO), reference.pos);
    }
}
