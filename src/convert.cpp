//
// Created by Tristan Krause on 2026-08-07.
//

#include "convert.hpp"
#include <charconv>
#include <optional>
#include <ranges>
#include <system_error>

#include "simulationerror.hpp"

using std::ranges::to;
using std::views::join_with;
using std::views::transform;

namespace
{
    std::optional<int> int_from_string_impl(std::string_view str)
    {
        if (str.empty()) { return std::nullopt; }

        int value = 0;
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

        // 1. Check for conversion errors
        if (ec == std::errc::invalid_argument)
        {
            // Not a number (e.g., "abc")
            return std::nullopt;
        }
        if (ec == std::errc::result_out_of_range)
        {
            // Exceeds min/max bounds of int
            return std::nullopt;
        }

        // 2. Check if the ENTIRE string was consumed (e.g., catches "123abc")
        if (ptr != str.data() + str.size()) { return std::nullopt; }

        return value;
    }
} // namespace

int convert::int_from_string(std::string_view str)
{
    auto const opt = int_from_string_impl(str);
    if (!opt) throw SimulationError("Cannot parse string to int: '{}'", str);
    return opt.value();
}

std::string convert::string_from_version(std::array<int, 3> const& version)
{
    return version | transform([](int v) { return std::to_string(v); }) | join_with('.') | to<std::string>();
}
