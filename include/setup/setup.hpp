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
    struct Setup final
    {
        using VarMap = std::map<std::string, Var>;

        [[nodiscard]] explicit Setup(std::filesystem::path const& path_json);
        [[nodiscard]] explicit Setup(ojson const& js_in, std::filesystem::path path_cwd = ".");

        Setup(const Setup& other) :
            timestamp_{other.timestamp_}, name_{other.name_}, variables_{other.variables_}, sim_params_{other.sim_params_}, references_{other.references_},
            antennas_{other.antennas_}, geometries_{other.geometries_}, sweeps_{other.sweeps_}, tasks_{other.tasks_}
        { reconcile(); }

        Setup(Setup&& other) :
            timestamp_{other.timestamp_}, name_{std::move(other.name_)}, variables_{std::move(other.variables_)}, sim_params_{std::move(other.sim_params_)},
            references_{std::move(other.references_)}, antennas_{std::move(other.antennas_)}, geometries_{std::move(other.geometries_)},
            sweeps_{std::move(other.sweeps_)}, tasks_{std::move(other.tasks_)}
        { reconcile(); }

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
            swap(lhs.sim_params_, rhs.sim_params_);
            swap(lhs.references_, rhs.references_);
            swap(lhs.antennas_, rhs.antennas_);
            swap(lhs.geometries_, rhs.geometries_);
            swap(lhs.sweeps_, rhs.sweeps_);
            swap(lhs.tasks_, rhs.tasks_);
            lhs.reconcile();
            rhs.reconcile();
        }

        // --------------------------------------------
        // simple getters
        // --------------------------------------------

        [[nodiscard]] std::string const& name() const { return name_; }

        [[nodiscard]] VarMap const& variables() const { return variables_; }

        [[nodiscard]] setup::SimParams const& sim_params() const { return sim_params_; }

        [[nodiscard]] std::span<const reference::Reference> references() const { return references_; }

        [[nodiscard]] std::span<const antenna::Antenna> antennas() const { return antennas_; }

        // --------------------------------------------

        void reconcile();
        void print_meta() const;
        void print_variables() const;
        void print_references() const;
        void print_antennas() const;
        void print_sim_params() const;
        void export_to_three(std::filesystem::path const& path_objects) const;
        void run_tasks(bool force_recomputation) const;

        [[nodiscard]] reference::Reference const& get_reference(std::string_view id) const;
        [[nodiscard]] antenna::Antenna const& get_antenna(std::string const& id) const;

        [[nodiscard]] std::span<reference::Reference const> get_references() const { return references_; }

        [[nodiscard]] std::span<antenna::Antenna const> get_antennas() const { return antennas_; }

        [[nodiscard]] double get_double(std::string const& variable_name) const;
        [[nodiscard]] std::int64_t get_int(std::string const& variable_name) const;

    private:
        void extract_meta(ojson& js);
        void extract_codebooks(ojson& js);
        void extract_sim_params(ojson& js);
        void extract_variables(ojson& js);
        void extract_references(ojson& js);
        void extract_antennas(ojson& js);
        void extract_geometries(ojson& js);
        void extract_sweeps(ojson& js);
        void extract_tasks(ojson& js);

        void run_task(task::Task const& task, std::filesystem::path const& path_output, eval::output::OutputType output_type) const;

        std::filesystem::path path_cwd_{};
        timeutil::timestamp_t timestamp_{};
        std::string name_;
        std::array<int, 3> version_;
        std::vector<Codebook> codebooks_;
        SimParams sim_params_;
        VarMap variables_;
        std::vector<reference::Reference> references_;
        std::vector<antenna::Antenna> antennas_;
        std::vector<geometry::Geometry> geometries_;
        std::vector<sweep::Sweep> sweeps_;
        std::vector<task::Task> tasks_;
    };
} // namespace setup
