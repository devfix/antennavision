//
// Created by Tristan Krause on 2026-05-26.
//

#pragma once

#include <filesystem>
#include <functional>
#include <list>

#include "components/radiator.hpp"
#include "components/radiatorarray.hpp"
#include "components/source.hpp"
#include "timeutil.hpp"
#include "types.hpp"

struct Setup
{
    struct RadiatorSetup
    {
        std::string type;
        pos_t position;
        double magnitude;
        double phase;
    };

    using task_t = std::function<void(std::filesystem::path const& directory)>;

    static std::unique_ptr<Setup> from_json(json const& js, timeutil::timestamp_t timestamp = 0);
    static std::unique_ptr<Setup> from_file(std::filesystem::path const& p);
    void export_to_three(std::filesystem::path const& directory) const;
    void run_tasks(std::filesystem::path const& directory, const std::function<void(std::string_view)>& builtin_handler);

    [[nodiscard]] Reference& get_reference_by_id(std::string_view id);
    [[nodiscard]] Radiator const& get_radiator_by_id(std::string_view id) const;

    [[nodiscard]] static std::complex<double> calc_voltage_gain(Radiator const& radiator_tx, Radiator const& radiator_rx, double wavelength, math::NumParams const& num_params);
    [[nodiscard]] static std::complex<double> calc_voltage_gain(RadiatorArray const& radiator_array_tx, Radiator const& radiator_rx, double wavelength, math::NumParams const& num_params);
    [[nodiscard]] static double calc_power_gain(Radiator const& radiator_tx, Radiator const& radiator_rx, double wavelength, math::NumParams const& num_params);
    [[nodiscard]] static double calc_power_gain(RadiatorArray const& radiator_array_tx, Radiator const& radiator_rx, double wavelength, math::NumParams const& num_params);

    std::string const name;
    timeutil::timestamp_t const timestamp;
    std::map<std::string, double> const variables;
    std::list<Reference> references;
    std::list<std::unique_ptr<Radiator>> const radiators;
    std::map<std::string, RadiatorArray> const radiator_arrays;
    std::list<std::pair<std::string, task_t>> const tasks;

    std::vector<Source> sources;
    std::vector<Component> inter_components;

private:
    Setup(std::string_view name, timeutil::timestamp_t timestamp, std::map<std::string, double>&& variables, std::list<Reference>&& references, std::list<std::unique_ptr<Radiator>>&& radiators, std::map<std::string, RadiatorArray> && radiator_arrays,
          std::list<std::pair<std::string, task_t>>&& tasks);
};
