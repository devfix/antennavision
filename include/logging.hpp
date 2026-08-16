//
// Created by Tristan Krause on 2026-08-16.
//

#pragma once
#include <format>
#include <string_view>

namespace antennavision::logging
{
    enum struct LogLevel
    {
        DEBUG,
        INFO,
        ALERT,
        WARN,
        ERROR,
    };

    constexpr std::string_view CLEAR_LINE = "\033[2K\r";

    bool enable_windows_ansi();

    void set_level(LogLevel level) noexcept;

    [[nodiscard]] LogLevel get_level() noexcept;

    void log_impl(LogLevel lvl, std::string_view component, std::string_view message, bool newline = true, bool clear_line = true);

    // -------------------------------------------------------------
    // Formatted Logging Wrappers (C++20 std::format)
    // -------------------------------------------------------------

    template <typename... Args>
    void log(LogLevel lvl, std::string_view component, std::format_string<Args...> fmt, Args&&... args)
    {
        if (lvl < get_level()) return;
        std::string msg = std::format(fmt, std::forward<Args>(args)...);
        log_impl(lvl, component, msg, false);
    }

    template <typename... Args>
    void logln(LogLevel lvl, std::string_view component, std::format_string<Args...> fmt, Args&&... args)
    {
        if (lvl < get_level()) return;
        std::string msg = std::format(fmt, std::forward<Args>(args)...);
        log_impl(lvl, component, msg, true);
    }

    template <typename... Args>
    void debug(std::string_view component, std::format_string<Args...> fmt, Args&&... args)
    { logln(LogLevel::DEBUG, component, fmt, std::forward<Args>(args)...); }

    template <typename... Args>
    void info(std::string_view component, std::format_string<Args...> fmt, Args&&... args)
    { logln(LogLevel::INFO, component, fmt, std::forward<Args>(args)...); }

    template <typename... Args>
    void alert(std::string_view component, std::format_string<Args...> fmt, Args&&... args)
    { logln(LogLevel::ALERT, component, fmt, std::forward<Args>(args)...); }

    template <typename... Args>
    void warn(std::string_view component, std::format_string<Args...> fmt, Args&&... args)
    { logln(LogLevel::WARN, component, fmt, std::forward<Args>(args)...); }

    template <typename... Args>
    void error(std::string_view component, std::format_string<Args...> fmt, Args&&... args)
    { logln(LogLevel::ERROR, component, fmt, std::forward<Args>(args)...); }

} // namespace antennavision::logging
