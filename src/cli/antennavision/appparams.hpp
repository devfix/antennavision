//
// Created by Tristan Krause on 2026-08-07.
//

#pragma once

#include <string>

struct AppParams
{
    bool print_version = false;
    bool debug_mode = false;
    bool quiet_mode = false;
    bool hide_banner = false;
    bool print_variables = false;
    bool print_references = false;
    bool print_antennas = false;
    bool run_tasks = false;
    bool force_recomputation = false;
    std::string path_setup{};
    std::string path_objects{};
};
