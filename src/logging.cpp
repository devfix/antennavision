//
// Created by Tristan Krause on 2026-08-16.
//

#include "logging.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <utility>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#endif

namespace antennavision::logging
{
    namespace ansi
    {
        // ===================================================
        // Control / Reset & Formatting
        // ===================================================
        constexpr std::string_view RESET = "\033[0m";
        constexpr std::string_view BOLD = "\033[1m";
        constexpr std::string_view DIM = "\033[2m";
        constexpr std::string_view ITALIC = "\033[3m";
        constexpr std::string_view UNDERLINE = "\033[4m";
        constexpr std::string_view BLINK = "\033[5m";
        constexpr std::string_view REVERSE = "\033[7m";
        constexpr std::string_view HIDDEN = "\033[8m";
        constexpr std::string_view STRIKETHROUGH = "\033[9m";

        // ===================================================
        // Standard Foreground Colors (30–37)
        // ===================================================
        constexpr std::string_view BLACK = "\033[30m";
        constexpr std::string_view RED = "\033[31m";
        constexpr std::string_view GREEN = "\033[32m";
        constexpr std::string_view YELLOW = "\033[33m";
        constexpr std::string_view BLUE = "\033[34m";
        constexpr std::string_view MAGENTA = "\033[35m";
        constexpr std::string_view CYAN = "\033[36m";
        constexpr std::string_view WHITE = "\033[37m";

        // ===================================================
        // Bright / High-Intensity Foreground Colors (90–97)
        // ===================================================
        constexpr std::string_view GRAY = "\033[90m"; // Bright Black / Dark Gray
        constexpr std::string_view BRIGHT_BLACK = "\033[90m";
        constexpr std::string_view BRIGHT_RED = "\033[91m";
        constexpr std::string_view BRIGHT_GREEN = "\033[92m";
        constexpr std::string_view BRIGHT_YELLOW = "\033[93m";
        constexpr std::string_view BRIGHT_BLUE = "\033[94m";
        constexpr std::string_view BRIGHT_MAGENTA = "\033[95m";
        constexpr std::string_view BRIGHT_CYAN = "\033[96m";
        constexpr std::string_view BRIGHT_WHITE = "\033[97m";

        // ===================================================
        // Standard Background Colors (40–47)
        // ===================================================
        constexpr std::string_view BG_BLACK = "\033[40m";
        constexpr std::string_view BG_RED = "\033[41m";
        constexpr std::string_view BG_GREEN = "\033[42m";
        constexpr std::string_view BG_YELLOW = "\033[43m";
        constexpr std::string_view BG_BLUE = "\033[44m";
        constexpr std::string_view BG_MAGENTA = "\033[45m";
        constexpr std::string_view BG_CYAN = "\033[46m";
        constexpr std::string_view BG_WHITE = "\033[47m";

        // ===================================================
        // Bright / High-Intensity Background Colors (100–107)
        // ===================================================
        constexpr std::string_view BG_GRAY = "\033[100m";
        constexpr std::string_view BG_BRIGHT_BLACK = "\033[100m";
        constexpr std::string_view BG_BRIGHT_RED = "\033[101m";
        constexpr std::string_view BG_BRIGHT_GREEN = "\033[102m";
        constexpr std::string_view BG_BRIGHT_YELLOW = "\033[103m";
        constexpr std::string_view BG_BRIGHT_BLUE = "\033[104m";
        constexpr std::string_view BG_BRIGHT_MAGENTA = "\033[105m";
        constexpr std::string_view BG_BRIGHT_CYAN = "\033[106m";
        constexpr std::string_view BG_BRIGHT_WHITE = "\033[107m";

    } // namespace ansi

    namespace
    {

        // Thread-safety guard
        std::mutex g_log_mutex;

        // Thread-safe atomic level filter
        std::atomic<LogLevel> g_current_level{LogLevel::DEBUG};

        struct LevelMetadata
        {
            std::string_view name;
            std::string_view color;
        };

        LevelMetadata get_level_metadata(LogLevel lvl)
        {
            switch (lvl)
            {
                case LogLevel::DEBUG: return {"DEBUG", ansi::BRIGHT_BLACK};
                case LogLevel::INFO: return {"INFO", ansi::RESET};
                case LogLevel::ALERT: return {"ALERT", ansi::BRIGHT_CYAN};
                case LogLevel::WARN: return {"WARN", ansi::YELLOW};
                case LogLevel::ERROR: return {"ERROR", ansi::RED};
                default: std::unreachable();
            }
        }

    } // anonymous namespace

#ifdef _WIN32
    bool enable_windows_ansi()
    {
        HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return false;

        DWORD target = 0;
        if (!::GetConsoleMode(hOut, &target)) return false;

        DWORD newMode = target | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!::SetConsoleMode(hOut, newMode)) return false;
        return true;
    }
#else
    bool enable_windows_ansi() { return true; }
#endif

    void set_level(LogLevel lvl) noexcept { g_current_level.store(lvl, std::memory_order_relaxed); }

    LogLevel get_level() noexcept { return g_current_level.load(std::memory_order_relaxed); }

    void log_impl(LogLevel lvl, std::string_view component, std::string_view message, bool newline, bool clear_line)
    {
        // 1. Get current local time (HH:MM:SS)
        auto const now = std::chrono::system_clock::now();
        auto const time = std::chrono::current_zone()->to_local(now);

        auto const meta = get_level_metadata(lvl);

        std::lock_guard _(g_log_mutex);

        // Format prefix: [14:32:01] [ScalarField] [INFO ]
        if (clear_line) std::cout << CLEAR_LINE;
        std::cout << std::format("[{:%H:%M:%S}] [{}] [{}{}{}] {}",
            std::chrono::floor<std::chrono::seconds>(time),
            component,
            meta.color,
            meta.name,
            ansi::RESET,
            message);

        if (newline)
            std::cout << '\n';
        else
            std::cout << std::flush; // Flush immediately for in-place '\r' updates
    }
} // namespace antennavision::logging
