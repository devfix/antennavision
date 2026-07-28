//
// Created by Tristan Krause on 2026-05-26.
//

#pragma once

#include <filesystem>
#include <functional>
#include <list>
#include "factory/find.hpp"
#include "factory/make.hpp"
#include "task.hpp"
#include "timeutil.hpp"

namespace setup
{
    struct Setup
    {
        using VarMap = std::map<std::string, Var>;

        [[nodiscard]] Setup(std::filesystem::path const& path_json, bool override_timestamp = false);
        [[nodiscard]] Setup(ojson const& js_in);

        Setup(const Setup& other) :
            timestamp_{other.timestamp_}, name_{other.name_}, variables_{other.variables_}, num_params_{other.num_params_}, references_{other.references_},
            antennas_{other.antennas_}, geometries_{other.geometries_}, sweeps_{other.sweeps_}, tasks_{other.tasks_}
        { validate(); }

        Setup(Setup&& other) :
            timestamp_{other.timestamp_}, name_{std::move(other.name_)}, variables_{std::move(other.variables_)}, num_params_{std::move(other.num_params_)},
            references_{std::move(other.references_)}, antennas_{std::move(other.antennas_)}, geometries_{std::move(other.geometries_)},
            sweeps_{std::move(other.sweeps_)}, tasks_{std::move(other.tasks_)}
        { validate(); }

        Setup& operator=(Setup other)
        {
            std::swap(*this, other);
            return *this;
        }

        friend void swap(Setup& lhs, Setup& rhs)
        {
            using std::swap;
            swap(lhs.timestamp_, rhs.timestamp_);
            swap(lhs.name_, rhs.name_);
            swap(lhs.variables_, rhs.variables_);
            swap(lhs.num_params_, rhs.num_params_);
            swap(lhs.references_, rhs.references_);
            swap(lhs.antennas_, rhs.antennas_);
            swap(lhs.geometries_, rhs.geometries_);
            swap(lhs.sweeps_, rhs.sweeps_);
            swap(lhs.tasks_, rhs.tasks_);
            lhs.validate();
            rhs.validate();
        }

        // --------------------------------------------
        // simple getters
        // --------------------------------------------

        [[nodiscard]] std::string const& name() const { return name_; }

        [[nodiscard]] VarMap const& variables() const { return variables_; }

        [[nodiscard]] setup::NumParams const& num_params() const { return num_params_; }

        [[nodiscard]] std::span<const reference::Reference> references() const { return references_; }

        [[nodiscard]] std::span<const antenna::Antenna> antennas() const { return antennas_; }

        // --------------------------------------------

        void validate();

        void export_to_three(std::filesystem::path const& directory, std::string_view objects_name = "setup") const;
        void run_tasks(std::filesystem::path const& path_cwd);

        [[nodiscard]] reference::Reference const& get_reference(std::string_view id);
        [[nodiscard]] antenna::Antenna const& get_antenna(std::string const& id);

        [[nodiscard]] std::span<reference::Reference const> get_references() { return references_; }

        [[nodiscard]] std::span<antenna::Antenna const> get_antennas() { return antennas_; }

        [[nodiscard]] bool isUpToDate(std::filesystem::path const& path_timestamp) const;

        [[nodiscard]] double get_double(std::string const& variable_name) const;
        [[nodiscard]] std::int64_t get_int(std::string const& variable_name) const;

    private:
        void extract_meta(ojson& js);
        void extract_num_params(ojson& js);
        void extract_variables(ojson& js);
        void extract_references(ojson& js);
        void extract_antennas(ojson& js);
        void extract_geometries(ojson& js);
        void extract_sweeps(ojson& js);
        void extract_tasks(ojson& desc);

        timeutil::timestamp_t timestamp_{};
        std::string name_;
        VarMap variables_;
        NumParams num_params_;
        std::vector<reference::Reference> references_;
        std::vector<antenna::Antenna> antennas_;
        std::vector<geometry::Geometry> geometries_;
        std::vector<sweep::Sweep> sweeps_;
        std::vector<task::Task> tasks_;
    };
} // namespace setup
