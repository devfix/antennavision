#include <CLI/CLI.hpp>
#include <NumCpp.hpp>
#include <ansi_color.hpp>
#include <optional>
#include <ranges>
#include "bitmap.hpp"
#include "convert.hpp"
#include "lg.hpp"
#include "manifest.hpp"
#include "parameters.hpp"
#include "setup/setup.hpp"

using ansi_color::fg4;
using ansi_color::reset;


namespace
{
#ifndef NDEBUG
    constexpr bool DEBUG_MODE = true;
#else
    constexpr bool DEBUG_MODE = false;
#endif

    void print_fatal_error()
    {
        // Route exception trace to stderr
        std::cout.flush(); // 'end' stdout: create new line and flush output
        std::cout.rdbuf(nullptr); // Disables std::cout
        lg::println(lg::error, "-------------------------------- FATAL ERROR  --------------------------------");
        lg::println(lg::error, "Error Stack Trace:");
    }

    void print_exception_chain(const std::exception& e, int level = 0)
    {
        lg::println(lg::error, "{}- {}", std::string(level * 2, ' '), e.what());

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

    std::optional<Parameters> parse_params(int argc, char* argv[])
    {
        Parameters params{};
        CLI::App app{"Electromagnetic Wave Propagation Simulator", std::string(APPLICATION_NAME)};
        app.add_flag("-v,--version", params.print_version, "Print version and exit");
        app.add_flag("-d,--debug", params.debug_mode, "Enable verbose debug output");
        app.add_flag("-n,--hide-banner", params.hide_banner, "Hide ascii art application banner");
        app.add_flag("-b,--variables", params.print_variables, "Print evaluated variables");
        app.add_flag("-c,--references", params.print_references, "Print constructed references");
        app.add_flag("-a,--antennas", params.print_antennas, "Print constructed antennas");
        app.add_flag("-r,--run-tasks", params.run_tasks, "Run setup tasks");
        app.add_flag("-f,--force", params.force_recomputation, "Force recomputation of all tasks");
        app.add_option("-s,--setup", params.path_setup, "Set path to setups");
        app.add_option("-o,--objects", params.path_objects, "Set path to objects (three) export");

        try
        {
            app.parse(argc, argv);
        }
        catch (const CLI::ParseError& e)
        {
            app.exit(e);
            return {};
        }

        if (params.debug_mode)
        {
            params.print_variables = true;
            params.print_references = true;
            params.print_antennas = true;
        }

        return params;
    }

    int run(int argc, char* argv[])
    {
        using std::filesystem::path;
        using std::filesystem::recursive_directory_iterator;

        ansi_color::enable_windows_ansi();

        auto const params_opt = parse_params(argc, argv);
        if (not params_opt) return EXIT_FAILURE;
        auto const params = params_opt.value();

        if (params.print_version)
        {
            lg::println(lg::info, "{} v.{}", APPLICATION_NAME, convert::string_from_version(APPLICATION_VERSION));
            return EXIT_SUCCESS;
        }

        if (not params.hide_banner) lg::println(lg::alert, "{}{} v.{}\n", BANNER, APPLICATION_NAME, convert::string_from_version(APPLICATION_VERSION));
        if (DEBUG_MODE) { lg::println(lg::warning, "Warning: Compiled in debug mode. This will severely increase the computation time!"); }

        if (params.debug_mode)
        {
            lg::println(lg::note, "print version:       {}", params.print_version);
            lg::println(lg::note, "debug mode:          {}", params.debug_mode);
            lg::println(lg::note, "print variables:     {}", params.print_variables);
            lg::println(lg::note, "print references:    {}", params.print_references);
            lg::println(lg::note, "print antennas:      {}", params.print_antennas);
            lg::println(lg::note, "run tasks:           {}", params.run_tasks);
            lg::println(lg::note, "force recomputation: {}", params.force_recomputation);
            lg::println(lg::note, "path setup file:     {}", params.path_setup.empty() ? "<unset>" : params.path_setup);
            lg::println(lg::note, "path objects file:   {}", params.path_objects.empty() ? "<unset>" : params.path_objects);
        }

        if (params.path_setup.empty())
        {
            lg::println(lg::error, "Missing path to setup file. Pass -h for help.");
            return EXIT_SUCCESS;
        }

        path const path_setup = std::filesystem::weakly_canonical(params.path_setup);
        if (not std::filesystem::exists(path_setup) or not std::filesystem::is_regular_file(path_setup))
        {
            lg::println(lg::error, "Could not find setup file: {}", path_setup.string());
            return EXIT_FAILURE;
        }

        setup::Setup su(path_setup);
        su.print_meta();
        if (params.print_variables) su.print_variables();
        if (params.print_references) su.print_references();
        if (params.print_antennas) su.print_antennas();
        if (not params.path_objects.empty()) su.export_to_three(params.path_objects);
        if (params.debug_mode) su.print_sim_params();
        if (params.run_tasks) su.run_tasks(params.force_recomputation);

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
        print_fatal_error();
        print_exception_chain(e);
    }
    catch (...)
    {
        print_fatal_error();
        lg::println(lg::error, "- [Unknown Critical Exception Caught]");
    }
    return EXIT_FAILURE;
}
