//
// Created by Tristan Krause on 2026-05-29.
//

#include "testutil.hpp"
#include <exception>
#include <ranges>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_translate_exception.hpp>

namespace {

    // Recursively unwrap and format nested exceptions using std::format & C++26 features
    void unwrap_nested_exception(const std::exception& e, std::string& out, int depth = 0) {
        std::string_view message{e.what()};

        // Use std::ranges::views::split to lazily iterate lines without string copies
        auto lines_view = message | std::views::split('\n')
                                  | std::views::transform([](auto&& rng) {
                                        return std::string_view(rng);
                                    });

        if (depth == 0) {
            // Top-level exception header
            std::format_to(std::back_inserter(out), "\n[Exception Chain]\n");

            bool first = true;
            for (std::string_view line : lines_view) {
                if (first) {
                    std::format_to(std::back_inserter(out), " [!] {}\n", line);
                    first = false;
                } else {
                    std::format_to(std::back_inserter(out), "     {}\n", line);
                }
            }
        } else {
            // Nested exception branch
            std::string indent(static_cast<size_t>(depth - 1) * 4, ' ');

            bool first = true;
            for (std::string_view line : lines_view) {
                if (first) {
                    std::format_to(std::back_inserter(out), "{} \\-- {}\n", indent, line);
                    first = false;
                } else {
                    std::format_to(std::back_inserter(out), "{}     {}\n", indent, line);
                }
            }
        }

        // Recurse into nested exception
        try {
            std::rethrow_if_nested(e);
        } catch (const std::exception& nested) {
            unwrap_nested_exception(nested, out, depth + 1);
        } catch (...) {
            std::string indent(static_cast<size_t>(depth) * 4, ' ');
            std::format_to(std::back_inserter(out), " {} \\-- [Unknown non-std::exception]\n", indent);
        }
    }

} // namespace

// Catch2 exception translator registration
CATCH_TRANSLATE_EXCEPTION(std::exception const& ex) {
    std::string result;
    unwrap_nested_exception(ex, result);
    return result;
}

void test_inverse_transformation(reference::Reference const &reference, Pos const &pos)
{
    CHECK_CLOSE_POSITION(reference.local_from_global_pos(reference.global_from_local_pos(pos)), pos);
    CHECK_CLOSE_POSITION(reference.global_from_local_pos(reference.local_from_global_pos(pos)), pos);
}

void test_basic_transformations(reference::Reference const &reference)
{
    test_inverse_transformation(reference, POS_ZERO);
    test_inverse_transformation(reference, reference.pos);
    if (reference.origin)
    {
        CHECK_CLOSE_POSITION(reference.local_from_global_pos(reference.origin->global_from_local_pos(reference.pos)), POS_ZERO);
        CHECK_CLOSE_POSITION(reference.global_from_local_pos(POS_ZERO), reference.origin->global_from_local_pos(reference.pos));
    }
    else
    {
        CHECK_CLOSE_POSITION(reference.local_from_global_pos(reference.pos), POS_ZERO);
        CHECK_CLOSE_POSITION(reference.global_from_local_pos(POS_ZERO), reference.pos);
    }
}
