//
// Created by core on 2026-10-31.
//

#include "appparams.hpp"
#include <CLI/CLI.hpp>
#include <print>

AppParams AppParams::parse(int argc, char* argv[])
{
    AppParams app_params{};
    CLI::App app{"Gain statistics of a ULA codebook at random receiver positions"};
    app.add_option("-c,--codebook,codebook", app_params.path_codebook, "Path to the codebook")->required()->check(CLI::ExistingFile);
    app.add_flag("--path-loss", app_params.enable_path_loss, "Enable path loss")->capture_default_str();
    app.add_option("--n-samples", app_params.n_samples, "Number of random receiver positions")->check(CLI::PositiveNumber)->capture_default_str();
    app.add_option("--n-sector-points", app_params.n_sector_points, "Number of points of the sector CCDF")->check(CLI::PositiveNumber)->capture_default_str();
    app.add_option("--n-service-points", app_params.n_service_points, "Number of points of the service CCDF")->check(CLI::PositiveNumber)->capture_default_str();
    app.add_option("--sector-ccdf-linear-min", app_params.sector_ccdf_linear_min, "")->check(CLI::NonNegativeNumber)->capture_default_str();
    app.add_option("--sector-ccdf-linear-max", app_params.sector_ccdf_linear_max, "")->capture_default_str();
    app.add_option("--sector-ccdf-decibel-min", app_params.sector_ccdf_decibel_min, "")->capture_default_str();
    app.add_option("--sector-ccdf-decibel-max", app_params.sector_ccdf_decibel_max, "")->capture_default_str();
    app.add_option("--service-ccdf-linear-min", app_params.service_ccdf_linear_min, "")->check(CLI::NonNegativeNumber)->capture_default_str();
    app.add_option("--service-ccdf-linear-max", app_params.service_ccdf_linear_max, "")->capture_default_str();
    app.add_option("--service-ccdf-decibel-min", app_params.service_ccdf_decibel_min, "")->capture_default_str();
    app.add_option("--service-ccdf-decibel-max", app_params.service_ccdf_decibel_max, "")->capture_default_str();
    app.add_option("-r,--result", app_params.path_result, "Path of the result file (.json, otherwise msgpack)")->required();
    app.add_option("-o,--objects", app_params.path_objects, "Set path to objects (three) export")->capture_default_str();

    if (argc <= 1)
    {
        std::println("{}", app.help());
        std::exit(0);
    }

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError& e)
    {
        std::exit(app.exit(e));
    }

    return app_params;
}

void AppParams::print() const
{
    std::println("App Parameters:");
    std::println("  path codebook: {}", path_codebook);
    std::println("  enable path loss: {}", enable_path_loss);
    std::println("  number of samples: {}", n_samples);
    std::println("  number of sector points: {}", n_sector_points);
    std::println("  number of service points: {}", n_service_points);
    std::println("  sector ccdf linear min: {}", sector_ccdf_linear_min);
    std::println("  sector ccdf linear max: {}", sector_ccdf_linear_max);
    std::println("  sector ccdf decibel min: {}", sector_ccdf_decibel_min);
    std::println("  sector ccdf decibel max: {}", sector_ccdf_decibel_max);
    std::println("  service ccdf linear min: {}", service_ccdf_linear_min);
    std::println("  service ccdf linear max: {}", service_ccdf_linear_max);
    std::println("  service ccdf decibel min: {}", service_ccdf_decibel_min);
    std::println("  service ccdf decibel max: {}", service_ccdf_decibel_max);
    std::println("  path result: {}", path_result);
    std::println("  path objects: {}", path_objects);
}
