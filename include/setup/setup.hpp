//
// Created by Tristan Krause on 2026-05-26.
//

#pragma once

#include <filesystem>
#include <functional>
#include <list>
#include "../eval/scalarfield.hpp"
#include "../factory/find.hpp"
#include "../factory/make.hpp"
#include "../timeutil.hpp"
#include "../types/math.hpp"

struct Setup
{
    using Task = std::function<void()>;
    using VarMap = std::map<std::string, Var>;
    using TaskMap = std::map<std::string, Task>;

    [[nodiscard]] Setup(std::filesystem::path const& path_json, bool override_timestamp = false);
    [[nodiscard]] Setup(ojson const& js_in);

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
    void run_tasks(const std::function<void(std::string_view)>& builtin_handler);

    [[nodiscard]] reference::Reference& get_reference(std::string_view id);
    [[nodiscard]] antenna::Antenna& get_antenna(std::string const& id);

    [[nodiscard]] std::span<reference::Reference> get_references() { return references_; }

    [[nodiscard]] std::span<antenna::Antenna> get_antennas() { return antennas_; }

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
    setup::NumParams num_params_;
    std::vector<reference::Reference> references_;
    std::vector<antenna::Antenna> antennas_;
    std::vector<geometry::Geometry> geometries_;
    std::vector<sweep::Sweep> sweeps_;

    TaskMap tasks_;
};
