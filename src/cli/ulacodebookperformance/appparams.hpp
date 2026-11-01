//
// Created by Tristan Krause on 2026-10-31.
//

#pragma once

#include <cstddef>
#include <string>

struct AppParams
{
    static AppParams parse(int argc, char* argv[]);
    void print() const;

    std::string path_codebook{};
    bool enable_path_loss = false;
    std::size_t n_points = 100'000;
    std::size_t n_sector_bins = 10;
    std::size_t n_service_bins = 100;
    float sector_ccdf_linear_min = 0.5;
    float sector_ccdf_linear_max= 1.0;
    float sector_ccdf_decibel_min = -3.0;
    float sector_ccdf_decibel_max = 0.0;
    float service_ccdf_linear_min = 0.0;
    float service_ccdf_linear_max = 1.0;
    float service_ccdf_decibel_min = -10.0;
    float service_ccdf_decibel_max = 0.0;
    std::string path_result{};
    std::string path_objects = "/tmp/objects-sfc.js";
};
