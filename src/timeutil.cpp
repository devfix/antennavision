//
// Created by core on 01.07.26.
//

#include "timeutil.hpp"
#include <chrono>
#include <fstream>
#include "simulationerror.hpp"

namespace timeutil
{
    namespace
    {
        timestamp_t timestamp_from_string(std::string const& str) {
            timestamp_t value = 0;
            if (auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value); ec == std::errc{}) {
                return value;
            }
            throw SimulationError("Invalid timestamp: {}", str);
        }
    }
    timestamp_t get_of_file(std::filesystem::path const& file_path)
    {
        if (!std::filesystem::exists(file_path)) { throw SimulationError("File not found: {}", file_path.string()); }
        std::filesystem::file_time_type const file_time = std::filesystem::last_write_time(file_path);
        auto system_time = std::chrono::clock_cast<std::chrono::system_clock>(file_time);
        return std::chrono::duration_cast<std::chrono::seconds>(system_time.time_since_epoch()).count();
    }

    timestamp_t load_from_file(std::filesystem::path const& file_path)
    {
        if (!std::filesystem::exists(file_path)) { throw SimulationError("File not found: {}", file_path.string()); }
        std::ifstream ifs(file_path);
        auto const str = std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        return timestamp_from_string(str);
    }

    void store_to_file(std::filesystem::path const& file_path, timestamp_t const timestamp)
    {
        std::ofstream ofs(file_path);
        if (!ofs.good())
        {
            throw SimulationError("Cannot open file for writing: {}", file_path.string());
        }
        ofs << std::to_string(timestamp);
    }

    std::string format(timestamp_t const timestamp)
    {
        std::chrono::seconds const duration(timestamp);
        std::chrono::sys_seconds time_point{duration};
        auto const local_zone = std::chrono::current_zone();
        auto local_time = local_zone->to_local(time_point);
        // %Y = Year, %m = Month, %d = Day, %H = Hour, %M = Minute, %S = Second
        return std::format("{:%Y-%m-%d %H:%M:%S}", local_time);
    }
} // namespace timeutil
