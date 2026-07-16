//
// Created by Tristan Krause on 2026-05-26.
//

#pragma once

#include <filesystem>
#include <functional>
#include <list>

#include "components/source.hpp"
#include "factory/find.hpp"
#include "factory/make.hpp"
#include "scalarfield.hpp"
#include "timeutil.hpp"
#include "types.hpp"

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

    std::string const name;
    timeutil::timestamp_t const timestamp;
    std::map<std::string, var_t> const variables;
    math::NumParams const num_params;
    std::list<Reference> references;
    std::map<std::string, Antenna> antennas;
    std::list<std::pair<std::string, task_t>> const tasks;

    std::vector<Source> sources;
    std::vector<Component> inter_components;

private:
    Setup(std::string_view name, timeutil::timestamp_t timestamp, factory::Context&& context);
};
