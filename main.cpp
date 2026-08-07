#include <print>
#include <ranges>
#include <CLI/CLI.hpp>
#include <NumCpp.hpp>
#include <ansi_color.hpp>
#include "bitmap.hpp"
#include "convert.hpp"
#include "setup/setup.hpp"
#include "manifest.hpp"
#include "parameters.hpp"

using ansi_color::fg4;
using ansi_color::reset;

/**
 *  TODO
 *  - add export three option
 *  - implement print variable/radiators etc logic
 *  - implement isotropical radiator
 *  - implement directivity task for arrays (at the moment only supported for radiators)
 *  - finally compile and tag the version :)
 */
namespace
{
#ifndef NDEBUG
    constexpr bool DEBUG_MODE = true;
#else
    constexpr bool DEBUG_MODE = false;
#endif

    void print_exception_chain(const std::exception& e, int level = 0)
    {
        std::println(std::cerr, "{}- {}", std::string(level * 2, ' '), e.what());

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

    int parse_params(int argc, char* argv[], Parameters& params)
    {
        CLI::App app{"Electromagnetic Wave Propagation Simulator", std::string(APPLICATION_NAME)};
        app.add_flag("-d,--debug", params.debug_mode, "Enable verbose debug output");
        app.add_flag("-v,--variables", params.print_variables, "Print evaluated variables");
        app.add_flag("-r,--radiators", params.print_radiators, "Print constructed radiators");
        app.add_option("-s,--setups", params.path_setups, "Set path to setups");
        CLI11_PARSE(app, argc, argv);

        if (params.debug_mode) params.print_variables = true;

        if (params.debug_mode)
        {
            std::println("{}debug mode:      {}{}", fg4::bright_black, params.debug_mode, reset);
            std::println("{}print variables: {}{}", fg4::bright_black, params.print_variables, reset);
            std::println("{}print radiators: {}{}", fg4::bright_black, params.print_radiators, reset);
            std::println("{}path to setups:  {}{}", fg4::bright_black, params.path_setups, reset);
        }

        return EXIT_SUCCESS;
    }

    int run(int argc, char* argv[])
    {
        using std::filesystem::path;
        using std::filesystem::recursive_directory_iterator;

        ansi_color::enable_windows_ansi();

        Parameters params{};
        if (int rc = parse_params(argc, argv, params); rc != EXIT_SUCCESS) return rc;

        if (params.print_banner) std::println("{}{}{} v.{}{}\n", fg4::cyan, BANNER, APPLICATION_NAME, convert::string_from_version(APPLICATION_VERSION), reset);
        if (DEBUG_MODE) { std::println("{}Warning: Compiled in debug mode. This will severely increase the computation time!{}\n", fg4::bright_yellow, reset); }

        path const path_setups_dir(std::filesystem::weakly_canonical(path(params.path_setups)));
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

} // namespace

int main(int argc, char* argv[])
{
    try
    {
        return run(argc, argv);
    }
    catch (const std::exception& e)
    {
        // Route exception trace to stderr
        std::cout.flush(); // 'end' stdout: create new line and flush output
        std::cout.rdbuf(nullptr); // Disables std::cout
        std::println(std::cerr, "{}-------------------------------- FATAL ERROR  --------------------------------{}", fg4::red, reset);
        std::println(std::cerr, "{}Error Stack Trace:{}", fg4::red, reset);
        print_exception_chain(e);
    }
    catch (...)
    {
        // Catch-all non-standard or third-party strange exceptions
        std::cout << std::endl; // 'end' stdout: create new line and flush output
        std::cout.rdbuf(nullptr); // Disables std::cout
        std::println(std::cerr, "{}-------------------------------- FATAL ERROR  --------------------------------{}", fg4::red, reset);
        std::println(std::cerr, "{}Error Stack Trace:\n- [Unknown Critical Exception Caught]{}", fg4::red, reset);
    }
    return EXIT_FAILURE;
}
