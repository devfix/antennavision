//
// Created by Tristan Krause on 2026-07-01.
//

#pragma once

#include <format>
#include <stdexcept>

struct SimulationError : std::runtime_error
{
    template <typename... Args>
    explicit SimulationError(std::format_string<Args...> fmt, Args&&... args) :
        std::runtime_error(std::vformat(fmt.get(), std::make_format_args(args...)))
    {}
};
