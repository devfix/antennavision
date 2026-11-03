//
// Created by core on 2026-10-31.
//

#pragma once

#include <cstddef>
#include <string>

struct AppParams
{
    static AppParams parse(int argc, char* argv[]);
    void print() const;

    std::string path_codebook{};
    double wavelength{};
    bool enable_path_loss = false;
    std::size_t n_samples = 100'000;
    std::size_t n_sector_points = 50;
    std::size_t n_service_points = 100;
    double sector_ccdf_linear_min = 0.5;
    double sector_ccdf_linear_max= 1.0;
    double sector_ccdf_decibel_min = -3.0;
    double sector_ccdf_decibel_max = 0.0;
    double service_ccdf_linear_min = 0.0;
    double service_ccdf_linear_max = 1.0;
    double service_ccdf_decibel_min = -10.0;
    double service_ccdf_decibel_max = 0.0;
    std::size_t n_snr_points = 41; ///< number of points of the SNR_0 grid of the capacity curves
    double snr_db_min = 0.0; ///< SNR_0 grid: SNR of an ideally focused beam, in dB
    double snr_db_max = 40.0;
    double outage_epsilon = 0.05; ///< outage probability of the outage metrics (eta_eps, outage capacity)
    std::string path_result{};
    std::string path_objects = "/tmp/objects-sfc.js";
};
