#include <NumCpp.hpp>
#include <ansi_color.hpp>
#include <print>
#include <ranges>
#include "bitmap.hpp"
#include "builtin.hpp"
#include "include/setup/setup.hpp"
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
    using std::filesystem::path;
    using std::filesystem::recursive_directory_iterator;
    using ansi_color::fg4;
    using ansi_color::reset;

    ansi_color::enable_windows_ansi();

    std::println("{}{}{} v.{}{}\n", fg4::cyan, BANNER, APPLICATION_NAME, APPLICATION_VERSION, reset);

    if (DEBUG_MODE)
    {
        std::println(
            "{}Warning: Compiled in debug mode. This will severely increase the computation time!{}\n", fg4::bright_yellow, reset);
    }

    if (argc == 1)
    {
        std::println("Usage: {} <setup_dir>", argv[0]);
        return 0;
    }

    path const path_setups_dir(std::filesystem::weakly_canonical(path(argv[1])));
    std::filesystem::create_directories(path_setups_dir);
    std::filesystem::current_path(path_setups_dir);
    std::println("Working directory: {}", std::filesystem::current_path().string());

    std::vector<std::pair<path, setup::Setup>> setups;
    for (const auto& entry : recursive_directory_iterator(path_setups_dir))
    {
        if (entry.path().filename() == "setup.json")
        {
            std::filesystem::current_path(entry.path().parent_path());
            setups.emplace_back(entry.path(), entry.path());
        }
    }

    for (auto& [path, setup] : setups)
    {
        std::filesystem::current_path(path.parent_path());
        setup.export_to_three(".");
        setup.run_tasks(path.parent_path());

        // std::filesystem::path const path_timestamp = "timestamp";
        // if (setup.isUpToDate(path_timestamp))
        // {
        //     std::println(
        //         "{}Setup '{}' is unchanged since {}, skipping{}", fg4::cyan, setup.name, timeutil::format(setup.timestamp), reset);
        // }
        // else
        // {
        //     std::println("{}Setup '{}' is new or updated, running{}", fg4::cyan, setup.name, reset);
        //     setup.export_to_three(".");
        //     setup.run_tasks([&setup](std::string_view const key) { builtin::FunctionRegistry::instance().call(std::string(key), *setup); });
        //     timeutil::store_to_file(path_timestamp, setup.timestamp);
        //     std::println("{}All tasks completed.{}", fg4::cyan, reset);
        // }
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
