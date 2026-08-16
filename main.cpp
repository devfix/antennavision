#include <optional>
#include <ranges>
#include <CLI/CLI.hpp>
#include <NumCpp.hpp>
#include "appparams.hpp"
#include "bitmap.hpp"
#include "convert.hpp"
#include "logging.hpp"
#include "manifest.hpp"
#include "setup/setup.hpp"

namespace
{
    constexpr std::string_view LOG_NAME = "main";
    
#ifndef NDEBUG
    constexpr bool DEBUG_BUILD = true;
#else
    constexpr bool DEBUG_BUILD = false;
#endif

    void print_fatal_error()
    {
        // Route exception trace to stderr
        std::cout.flush(); // 'end' stdout: create new line and flush output
        std::cout.rdbuf(nullptr); // Disables std::cout
        antennavision::logging::error(LOG_NAME, "-------------------------------- FATAL ERROR  --------------------------------");
        antennavision::logging::error(LOG_NAME, "Error Stack Trace:");
    }

    void print_exception_chain(const std::exception& e, int level = 0)
    {
        antennavision::logging::error(LOG_NAME, "{}- {}", std::string(level * 2, ' '), e.what());
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

    std::optional<AppParams> parse_params(int argc, char* argv[])
    {
        AppParams app_params{};
        CLI::App app{"Electromagnetic Wave Propagation Simulator", std::string(APPLICATION_NAME)};
        app.add_flag("-v,--version", app_params.print_version, "Print version and exit");
        app.add_flag("-d,--debug", app_params.debug_mode, "Enable verbose debug output");
        app.add_flag("-q,--quiet", app_params.quiet_mode, "Quiet mode (less output)");
        app.add_flag("-n,--hide-banner", app_params.hide_banner, "Hide ascii art application banner");
        app.add_flag("-b,--variables", app_params.print_variables, "Print evaluated variables");
        app.add_flag("-c,--references", app_params.print_references, "Print constructed references");
        app.add_flag("-a,--antennas", app_params.print_antennas, "Print constructed antennas");
        app.add_flag("-r,--run-tasks", app_params.run_tasks, "Run setup tasks");
        app.add_flag("-f,--force", app_params.force_recomputation, "Force recomputation of all tasks");
        app.add_option("-s,--setup", app_params.path_setup, "Set path to setups");
        app.add_option("-o,--objects", app_params.path_objects, "Set path to objects (three) export");

        try
        {
            app.parse(argc, argv);
        }
        catch (const CLI::ParseError& e)
        {
            app.exit(e);
            return {};
        }

        if (app_params.debug_mode)
        {
            app_params.quiet_mode = false;
            app_params.print_variables = true;
            app_params.print_references = true;
            app_params.print_antennas = true;
        }

        return app_params;
    }

    int run(int argc, char* argv[])
    {
        using std::filesystem::path;
        using std::filesystem::recursive_directory_iterator;

        antennavision::logging::enable_windows_ansi();

        auto const params_opt = parse_params(argc, argv);
        if (not params_opt) return EXIT_FAILURE;
        auto const params = params_opt.value();

        if (params.quiet_mode and params.debug_mode)
        {
            antennavision::logging::error(LOG_NAME, "Quiet mode and debug mode can not be enabled simultaneously");
            return EXIT_FAILURE;
        }

        if (params.print_version)
        {
            antennavision::logging::info(LOG_NAME, "{} v.{}", APPLICATION_NAME, convert::string_from_version(APPLICATION_VERSION));
            return EXIT_SUCCESS;
        }

        // set correct logging level
        if (params.quiet_mode)
            antennavision::logging::set_level(antennavision::logging::LogLevel::WARN);
        else if (params.debug_mode)
            antennavision::logging::set_level(antennavision::logging::LogLevel::DEBUG);
        else
            antennavision::logging::set_level(antennavision::logging::LogLevel::INFO);

        if (DEBUG_BUILD) antennavision::logging::warn(LOG_NAME, "Warning: Compiled in debug mode. This will severely increase the computation time!");

        antennavision::logging::debug(LOG_NAME, "print version:       {}", params.print_version);
        antennavision::logging::debug(LOG_NAME, "debug mode:          {}", params.debug_mode);
        antennavision::logging::debug(LOG_NAME, "quiet mode:          {}", params.quiet_mode);
        antennavision::logging::debug(LOG_NAME, "hide banner:         {}", params.hide_banner);
        antennavision::logging::debug(LOG_NAME, "print variables:     {}", params.print_variables);
        antennavision::logging::debug(LOG_NAME, "print references:    {}", params.print_references);
        antennavision::logging::debug(LOG_NAME, "print antennas:      {}", params.print_antennas);
        antennavision::logging::debug(LOG_NAME, "run tasks:           {}", params.run_tasks);
        antennavision::logging::debug(LOG_NAME, "force recomputation: {}", params.force_recomputation);
        antennavision::logging::debug(LOG_NAME, "path setup file:     {}", params.path_setup.empty() ? "<unset>" : params.path_setup);
        antennavision::logging::debug(LOG_NAME, "path objects file:   {}", params.path_objects.empty() ? "<unset>" : params.path_objects);

        if (params.path_setup.empty())
        {
            antennavision::logging::error(LOG_NAME, "Missing path to setup file. Pass -h for help.");
            return EXIT_SUCCESS;
        }

        path const path_setup = std::filesystem::weakly_canonical(params.path_setup);
        if (not std::filesystem::exists(path_setup) or not std::filesystem::is_regular_file(path_setup))
        {
            antennavision::logging::error(LOG_NAME, "Could not find setup file: {}", path_setup.string());
            return EXIT_FAILURE;
        }

        setup::Setup su(path_setup);
        if (not params.quiet_mode) su.print_meta();
        if (params.print_variables) su.print_variables();
        if (params.print_references) su.print_references();
        if (params.print_antennas) su.print_antennas();
        if (not params.path_objects.empty()) su.export_to_three(params.path_objects);
        if (params.debug_mode) su.print_sim_params();
        if (params.run_tasks)
        {
            if (not params.hide_banner)
                antennavision::logging::alert(LOG_NAME, "{}{} v.{}\n", BANNER, APPLICATION_NAME, convert::string_from_version(APPLICATION_VERSION));
            su.run_tasks(params.force_recomputation);
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
        print_fatal_error();
        print_exception_chain(e);
    }
    catch (...)
    {
        print_fatal_error();
        antennavision::logging::error(LOG_NAME, "- [Unknown Critical Exception Caught]");
    }
    return EXIT_FAILURE;
}
