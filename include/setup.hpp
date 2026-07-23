//
// Created by Tristan Krause on 2026-05-26.
//

#pragma once

#include <filesystem>
#include <functional>
#include <list>
#include "factory/find.hpp"
#include "factory/make.hpp"
#include "scalarfield.hpp"
#include "timeutil.hpp"
#include "types/math.hpp"

struct Setup
{
    using task_t = factory::task_t;

    [[nodiscard]] static std::unique_ptr<Setup> from_json(ojson const& js, timeutil::timestamp_t timestamp = 0);
    [[nodiscard]] static std::unique_ptr<Setup> from_file(std::filesystem::path const& p);
    void export_to_three(std::filesystem::path const& directory, std::string_view objects_name = "setup") const;
    void run_tasks(const std::function<void(std::string_view)>& builtin_handler);

    [[nodiscard]] Reference& get_reference(std::string_view id);
    [[nodiscard]] Antenna& get_antenna(std::string const& id);

    [[nodiscard]] bool isUpToDate(std::filesystem::path const& path_timestamp) const;

    [[nodiscard]] std::string const& name() const { return name_; }

    [[nodiscard]] std::map<std::string, var_t> const& variables() const { return variables_; }

    [[nodiscard]] math::NumParams const& num_params() const { return num_params_; }

    [[nodiscard]] std::span<const Reference> references() const { return references_; }

    [[nodiscard]] std::span<const Antenna> antennas() const { return antennas_; }

    [[nodiscard]] double get_double(std::string const& variable_name) const;
    [[nodiscard]] std::int64_t get_int(std::string const& variable_name) const;

private:
    Setup(std::string_view name, timeutil::timestamp_t timestamp, factory::Context&& context);

    std::string name_;
    timeutil::timestamp_t timestamp_;
    std::map<std::string, var_t> variables_;
    math::NumParams num_params_;
    std::vector<Reference> references_;
    std::vector<Antenna> antennas_;
    std::list<std::pair<std::string, task_t>> tasks_;
};
