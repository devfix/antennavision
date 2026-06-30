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
#include "types.hpp"

struct Setup
{

    struct RadiatorSetup
    {
        std::string type;
        Vec3 position;
        double magnitude;
        double phase;
    };

    static std::unique_ptr<Setup> from_json(json const& js);
    static std::unique_ptr<Setup> from_file(std::filesystem::path const& p);
    void export_to_three() const;
    void run_tasks(const std::function<void(std::string_view)>& builtin_handler);

    [[nodiscard]] Reference& get_reference_by_id(std::string_view id);
    [[nodiscard]] Radiator const& get_radiator_by_id(std::string_view id) const;
    // [[nodiscard]] std::vector<std::reference_wrapper<Radiator>> get_radiator_array(std::string_view id) const;

    [[nodiscard]] static std::complex<double> calc_voltage_gain(Radiator const& radiator_tx, Radiator const& radiator_rx, double wavelength, math::NumParams const& num_params);
    [[nodiscard]] static std::complex<double> calc_voltage_gain(RadiatorArray const& radiator_array_tx, Radiator const& radiator_rx, double wavelength, math::NumParams const& num_params);
    [[nodiscard]] static double calc_power_gain(Radiator const& radiator_tx, Radiator const& radiator_rx, double wavelength, math::NumParams const& num_params);
    [[nodiscard]] static double calc_power_gain(RadiatorArray const& radiator_array_tx, Radiator const& radiator_rx, double wavelength, math::NumParams const& num_params);

    std::string const name;
    std::map<std::string, double> const variables;
    std::list<Reference> references;
    std::list<std::unique_ptr<Radiator>> const radiators;
    std::map<std::string, RadiatorArray> const radiator_arrays;
    std::list<std::pair<std::string, std::function<void()>>> const tasks;

    std::vector<Source> sources;
    std::vector<Component> inter_components;

private:
    Setup(std::string_view name, std::map<std::string, double>&& variables, std::list<Reference>&& references, std::list<std::unique_ptr<Radiator>>&& radiators, std::map<std::string, RadiatorArray> && radiator_arrays,
          std::list<std::pair<std::string, std::function<void()>>>&& tasks);
};
