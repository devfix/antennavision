//
// Created by core on 01.07.26.
//

#pragma once

#include <cstdint>
#include <filesystem>

namespace timeutil
{
    using timestamp_t = std::int64_t;
    timestamp_t get_of_file(std::filesystem::path const& file_path);
    timestamp_t load_from_file(std::filesystem::path const& file_path);
    void store_to_file(std::filesystem::path const& file_path, timestamp_t timestamp);
    std::string format(timestamp_t timestamp);
} // namespace timeutil
