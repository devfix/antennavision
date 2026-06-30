//
// Created by Tristan Krause on 2026-06-09.
//

#pragma once

#include <format>
#include <iostream>

namespace std
{
    // Drop-in replacement for C++23 std::print
    template <typename... Args>
    void print(std::format_string<Args...> fmt, Args&&... args)
    { std::cout << std::format(fmt, std::forward<Args>(args)...); }

    // Drop-in replacement for C++23 std::println
    template <typename... Args>
    void println(std::format_string<Args...> fmt, Args&&... args)
    { std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n'; }

    // Optional: Overload for printing a blank line
    inline void println() { std::cout << '\n'; }
} // namespace std
