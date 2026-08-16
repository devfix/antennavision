//
// Created by Tristan Krause on 2026-05-26.
//

#pragma once

#include <filesystem>
#include <functional>
#include <list>

#include "eval/output.hpp"
#include "factory/find.hpp"
#include "factory/make.hpp"
#include "task.hpp"
#include "timeutil.hpp"

namespace setup
{
    /**
     * Class "Radiator" of Aggregate OutputType
     * Also known as POD (Plain Old Data) / PDS (Passive Data Structure) / DTO (Data Transfer Object)
     */
    struct Setup
    {
        struct Meta
        {
            std::string name;
            std::array<int, 3> version;
            timeutil::timestamp_t timestamp;
        };

        [[nodiscard]] static  Setup from_json(ojson const& desc, std::filesystem::path const& cwd = ".", timeutil::timestamp_t timestamp = 0);
        [[nodiscard]] static  Setup from_file(std::filesystem::path const& path_json);

        void reconcile();
        void print_meta() const;
        void print_variables() const;
        void print_references() const;
        void print_antennas() const;
        void print_sim_params() const;
        void export_to_three(std::filesystem::path const& path_objects) const;
        void run_tasks(bool force_recomputation) const;

        [[nodiscard]] reference::Reference const& get_reference(std::string_view id);
        [[nodiscard]] antenna::Antenna const& get_antenna(std::string_view id);
        [[nodiscard]] Context get_context() const;
        [[nodiscard]] double get_double(std::string const& variable_name) const;
        [[nodiscard]] std::int64_t get_int(std::string const& variable_name) const;

        std::filesystem::path cwd;
        Meta meta;
        std::vector<Codebook> codebooks;
        SimParams sim_params;
        VarMap variables;
        std::vector<reference::Reference> references;
        std::vector<antenna::Antenna> antennas;
        std::vector<geometry::Geometry> geometries;
        std::vector<sweep::Sweep> sweeps;
        std::vector<task::Task> tasks;
    };
} // namespace setup
