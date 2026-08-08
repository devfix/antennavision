//
// Created by Tristan Krause on 2026-08-08.
//

#pragma once

#include <ansi_color.hpp>
#include <cstdio>
#include <format>

namespace lg
{
    auto constexpr note = ansi_color::fg4::bright_black;
    auto constexpr info = ansi_color::reset;
    auto constexpr alert = ansi_color::fg4::cyan;
    auto constexpr error = ansi_color::fg4::red;
    auto constexpr warning = ansi_color::fg4::bright_yellow;
    using ansi_color::csi::sgr::Code;

    // 1. Primary template defaults to false
    template <typename T>
    struct is_code : std::false_type
    {};

    // 2. Partial specialization matches any instance of Code<N>
    template <int N>
    struct is_code<Code<N>> : std::true_type
    {};

    // 3. Helper variable template
    template <typename T>
    inline constexpr bool is_code_v = is_code<std::remove_cvref_t<T>>::value;

    // 4. C++20 Concept
    template <typename T>
    concept IsCode = is_code_v<T>;

    template <IsCode Color, typename... Args>
    void print(Color color, std::format_string<Args...> fmt, Args&&... args)
    {
        auto const str = std::format("{}{}{}", color, std::vformat(fmt.get(), std::make_format_args(args...)), ansi_color::reset);
        std::fwrite(str.data(), sizeof(char), str.size(), stdout);
    }

    template <IsCode Color, typename... Args>
    void println(Color color, std::format_string<Args...> fmt, Args&&... args)
    {
        auto const str = std::format("{}{}{}\n", color, std::vformat(fmt.get(), std::make_format_args(args...)), ansi_color::reset);
        std::fwrite(str.data(), sizeof(char), str.size(), stdout);
    }

    template <typename... Args>
    void print(std::format_string<Args...> fmt, Args&&... args)
    {
        auto const str = std::format("{}{}{}", info, std::vformat(fmt.get(), std::make_format_args(args...)), ansi_color::reset);
        std::fwrite(str.data(), sizeof(char), str.size(), stdout);
    }

    template <typename... Args>
    void println(std::format_string<Args...> fmt, Args&&... args)
    {
        auto const str = std::format("{}{}{}\n", info, std::vformat(fmt.get(), std::make_format_args(args...)), ansi_color::reset);
        std::fwrite(str.data(), sizeof(char), str.size(), stdout);
    }
} // namespace lg
