#include <NumCpp.hpp>
#include <ansi_color.hpp>
#include <execution>
#include <ranges>
#include "bitmap.hpp"
#include "builtin.hpp"
#include "include/setup.hpp"
#include "manifest.hpp"
#include "plot.hpp"
#include "print.hpp"
#include "simulationerror.hpp"
#include "types.hpp"

namespace
{
#ifndef NDEBUG
    constexpr bool DEBUG_MODE = true;
#else
    constexpr bool DEBUG_MODE = false;
#endif
} // namespace

void run_builtin_task(Setup& setup, std::string_view key)
{
    builtin::FunctionRegistry::instance().call(std::string(key), setup);
    // if (key == "t00_ula_beamwidth") { builtin::t00_ula_beamwidth(setup); }
    // else if (key == "t00_upa_beam_shape") { builtin::t01_upa_beam_shape(setup); }
    // else
    // {
    //     throw SimulationError("Invalid builtin task key: {}", key);
    // }
}

int main(int argc, char* argv[])
{
    ansi_color::enable_windows_ansi();
    std::println("{}{}{} v.{}{}\n", ansi_color::fg4::cyan, BANNER, APPLICATION_NAME, APPLICATION_VERSION, ansi_color::reset);

    if (DEBUG_MODE)
    {
        std::println("{}Warning: Compiled in debug mode. This will severely increase the computation time!{}\n", ansi_color::fg4::bright_yellow,
                     ansi_color::reset);
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
        if (setup->isUpToDate(path_timestamp)) { std::println("{}Setup '{}' is unchanged since {}, skipping{}", ansi_color::fg4::cyan, setup->name, timeutil::format(setup->timestamp), ansi_color::reset); }
        else
        {
            std::println("{}Setup '{}' is new or updated, running{}", ansi_color::fg4::cyan, setup->name, ansi_color::reset);
            setup->export_to_three(".");
            setup->run_tasks([&setup](std::string_view const key) { run_builtin_task(*setup, key); });
            timeutil::store_to_file(path_timestamp, setup->timestamp);
            std::println("{}All tasks completed.{}", ansi_color::fg4::cyan, ansi_color::reset);
        }
    }

    return 0;

    std::size_t n = std::stoi(argv[1]);
    std::print("using {} values per dimension\n", n);

    double freq = 1e9;
    double lambda = SPEED_OF_LIGHT / freq;
    std::print("lambda: {:.02f} m\n", lambda);

    /*

    // auto r1 = IsotropicRadiator(complex_t{0, 0}, 0, freq);
    // auto r2 = TestRadiator(complex_t{0, 1}, std::numbers::pi/4);
    auto r3 = ULA<IsotropicRadiator>{complex_t{0, 0}, 0*std::numbers::pi/4, 8, lambda/2};
    std::vector<IRadiator*> const radiators = {&r3};

    double s = 100;

    compute_rect(
        nc::linspace(-s, s, n),
        nc::linspace(-s, s, n),
        radiators,
        freq);
    */

    return 0;
}
