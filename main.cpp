#include <NumCpp.hpp>
#include <ansi_color.hpp>
#include <print>
#include <ranges>
#include "bitmap.hpp"
#include "builtin.hpp"
#include "include/setup.hpp"
#include "manifest.hpp"

namespace
{
#ifndef NDEBUG
    constexpr bool DEBUG_MODE = true;
#else
    constexpr bool DEBUG_MODE = false;
#endif
} // namespace

void print_exception_chain(const std::exception& e, int level = 0)
{
    std::println("{}- {}", std::string(level * 2, ' '), e.what());

    try
    {
        std::rethrow_if_nested(e);
    }
    catch (const std::exception& nested)
    {
        print_exception_chain(nested, level + 1); // Recurse into the inner exception
    }
    catch (...)
    {}
}

int run(int argc, char* argv[])
{
    ansi_color::enable_windows_ansi();

    std::println("{}{}{} v.{}{}\n", ansi_color::fg4::cyan, BANNER, APPLICATION_NAME, APPLICATION_VERSION, ansi_color::reset);

    if (DEBUG_MODE)
    {
        std::println(
            "{}Warning: Compiled in debug mode. This will severely increase the computation time!{}\n", ansi_color::fg4::bright_yellow, ansi_color::reset);
    }

    if (argc == 1)
    {
        std::println("Usage: {} <setup_dir>", argv[0]);
        return 0;
    }

    std::filesystem::path const path_setups_dir(std::filesystem::weakly_canonical(std::filesystem::path(argv[1])));
    std::filesystem::create_directories(path_setups_dir);
    std::filesystem::current_path(path_setups_dir);
    std::println("Working directory: {}", std::filesystem::current_path().string());

    std::vector<std::pair<std::filesystem::path, std::unique_ptr<Setup>>> setups;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path_setups_dir))
    {
        if (entry.path().filename() == "setup.json")
        {
            std::filesystem::current_path(entry.path().parent_path());
            setups.emplace_back(entry.path(), nullptr);
        }
    }
    std::println("Loading {} setups", setups.size());
    for (auto& [path, setup] : setups) { setup = Setup::from_file(path); }

    for (auto& [path, setup] : setups)
    {
        std::filesystem::current_path(path.parent_path());

        std::filesystem::path const path_timestamp = "timestamp";
        if (setup->isUpToDate(path_timestamp))
        {
            std::println(
                "{}Setup '{}' is unchanged since {}, skipping{}", ansi_color::fg4::cyan, setup->name, timeutil::format(setup->timestamp), ansi_color::reset);
        }
        else
        {
            std::println("{}Setup '{}' is new or updated, running{}", ansi_color::fg4::cyan, setup->name, ansi_color::reset);
            setup->export_to_three(".");
            setup->run_tasks([&setup](std::string_view const key) { builtin::FunctionRegistry::instance().call(std::string(key), *setup); });
            timeutil::store_to_file(path_timestamp, setup->timestamp);
            std::println("{}All tasks completed.{}", ansi_color::fg4::cyan, ansi_color::reset);
        }
    }

    return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
    try
    {
        return run(argc, argv);
    }
    catch (const std::exception& e)
    {
        // Route to stderr using stderr as the first argument
        std::println(stderr, "Error Stack Trace:");
        print_exception_chain(e);
    }
    catch (...)
    {
        // Catch-all non-standard or third-party strange exceptions
        std::println(stderr, "Error Stack Trace:\n- [Unknown Critical Exception Caught]");
    }
    return EXIT_FAILURE;
}
