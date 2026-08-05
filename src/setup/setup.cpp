//
// Created by Tristan Krause on 2026-05-26.
//

#include "setup/setup.hpp"
#include <ansi_color.hpp>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <print>
#include "eval/output.hpp"
#include "factory/find.hpp"
#include "factory/get.hpp"
#include "factory/make.hpp"
#include "factory/parse.hpp"
#include "simulationerror.hpp"
#include "three.hpp"

namespace setup
{
    using ansi_color::fg4;
    using ansi_color::reset;
    using reference::Reference;

    namespace
    {
        ojson load_json(std::filesystem::path const& path_json)
        {
            std::println("{}Loading setup file '{}'{}", fg4::cyan, path_json.string(), reset);
            std::ifstream file(path_json);
            if (!file.is_open()) { throw SimulationError("Could not open setup file '{}'", path_json.string()); }
            auto const js = nlohmann::ordered_json::parse(file);
            file.close();
            return js;
        }

        std::vector<Complex> get_coeffs_from_codebook(std::span<Codebook const> codebooks, std::span<std::string const> key)
        {
            auto const& id_cb = key.at(0);
            auto it = std::ranges::find(codebooks, id_cb, &Codebook::id);
            if (it == codebooks.end()) throw SimulationError("Could not find codebook with id '{}'", id_cb);
            return (*it)[key.subspan(1)] | std::ranges::to<std::vector<Complex>>();
        }
    } // namespace

    Setup::Setup(std::filesystem::path const& path_json, bool override_timestamp) : Setup(load_json(path_json))
    { timestamp_ = timeutil::get_of_file(path_json); }

    Setup::Setup(ojson const& js_in)
    {
        ojson js = js_in; // create a copy of the json object in order to decompose it
        extract_meta(js);
        extract_codebooks(js);
        extract_num_params(js);
        extract_variables(js);
        extract_references(js);
        extract_antennas(js);
        extract_geometries(js);
        extract_sweeps(js);
        extract_tasks(js);

        // check that the json contains no invalid (unknown) fields
        factory::assert_empty(js);

        validate();
    }

    void Setup::validate()
    {
        // crucial: trace all origins by their id and connect the pointers
        reference::resolve_origins(references_);
        antenna::resolve_origins(antennas_, references_);

        // check that objects exists
        for (auto const& task : tasks_)
        {
            if (auto const t = std::get_if<task::DirectivityOverPolarAtAzimuth>(&task))
            {
                (void)antenna::get(antennas_, t->antenna_id);
                (void)sweep::get(sweeps_, t->sweep_id);
            }
            if (auto const t = std::get_if<task::RxVoltageFieldAtWavelength>(&task))
            {
                (void)antenna::get(antennas_, t->tx_id);
                (void)antenna::get(antennas_, t->rx_id);
                (void)geometry::get(geometries_, t->geo_id);
                (void)sweep::get(sweeps_, t->sweep_wavelength_id);
            }
        }
    }

    void Setup::export_to_three(std::filesystem::path const& directory, std::string_view const objects_name) const
    {
        std::filesystem::path const p = directory / std::format("{}.objects.js", objects_name);

        double const system_wavelength = num_params_.system_wavelength;

        three::Container container;
        for (auto const& reference : references_)
        {
            if (!reference.origin) { continue; } // we skip the dummy reference
            auto const pos_center = reference.global_from_local_pos(POS_ZERO);
            auto const pos_origin = reference.origin->global_from_local_pos(POS_ZERO);
            // auto const distance = (pos_center - pos_origin).norm();
            container.add(three::make_line(pos_origin, pos_center, 1.0, Color::white));
            container.add(three::create_coordinate_arrows(pos_center,
                reference.global_from_local_pos({1, 0, 0}) - pos_center,
                reference.global_from_local_pos({0, 1, 0}) - pos_center,
                reference.global_from_local_pos({0, 0, 1}) - pos_center,
                0.25 * system_wavelength));
        }

        auto add_radiator = [&container, &system_wavelength](Radiator const& radiator)
        {
            auto pos_center = radiator.origin->global_from_local_pos(POS_ZERO);
            double const radiator_length = 0.49 * system_wavelength;
            auto pos_end = radiator.origin->global_from_local_pos({0.0, 0.0, 0.5 * radiator_length});
            auto const pos_start = pos_center - (pos_end - pos_center);
            double const radius = 0.1 * radiator_length;
            container.add(three::make_cylinder(pos_start, pos_end, radius, radius));
        };
        for (auto const& antenna : antennas_)
        {
            std::visit(
                [&](auto const& ant)
                {
                    using Type = std::decay_t<decltype(ant)>;
                    if constexpr (std::is_same_v<Type, Radiator>) { add_radiator(ant); }
                    else if constexpr (std::is_base_of_v<RadiatorArray<Type>, Type>)
                    {
                        for (auto const& element : ant.elements) { add_radiator(element); }
                    }
                    else
                    {
                        throw SimulationError("Invalid antenna type");
                    }
                },
                antenna);
        }

        for (auto const& geo : geometries_) container.add(three::export_geometry(geo));
        container.export_to_javascript(p);
    }

    void Setup::run_tasks(std::filesystem::path const& path_cwd)
    {
        for (auto const& task : tasks_)
        {
            auto const id = setup::task::get_id(task);
            std::filesystem::path const path_json = path_cwd / (id + ".result.json");
            std::println("{}Running task: {}{}", fg4::cyan, id, reset);
            if (std::holds_alternative<task::DirectivityOverPolarAtAzimuth>(task))
            {
                auto& t = std::get<task::DirectivityOverPolarAtAzimuth>(task);
                auto& ant = antenna::get(antennas_, t.antenna_id);
                auto& sweep = sweep::get(sweeps_, t.sweep_id);
                eval::output::directivity_over_polar(t.path_output, ant, sweep, num_params_);
            }
            else if (std::holds_alternative<task::RxVoltageFieldAtWavelength>(task))
            {
                auto& t = std::get<task::RxVoltageFieldAtWavelength>(task);

                auto& tx = antenna::get(antennas_, t.tx_id);
                auto& rx = antenna::get(antennas_, t.rx_id);

                auto const tx_coeffs = t.tx_codebook.empty() //
                    ? std::vector<Complex>(antenna::size(tx), 1.0)
                    : get_coeffs_from_codebook(codebooks_, t.tx_codebook);

                auto const rx_coeffs = t.rx_codebook.empty() //
                    ? std::vector<Complex>(antenna::size(rx), 1.0)
                    : get_coeffs_from_codebook(codebooks_, t.rx_codebook);

                auto const& ref = reference::get(const_cast<decltype(references_) const&>(references_), t.ref_id);
                auto const field = eval::RxVoltageField(tx, rx, tx_coeffs, rx_coeffs, num_params_);
                auto const& geo = geometry::get(geometries_, t.geo_id);
                auto const& sweep = sweep::get(sweeps_, t.sweep_wavelength_id);

                eval::output::complex_scalarfield_at_wavelength(t.path_output, t, ref, field, geo, sweep);
            }
        }
    }

    Reference const& Setup::get_reference(std::string_view const id) { return factory::find_reference_by_id(references_, id); }

    antenna::Antenna const& Setup::get_antenna(std::string const& id) { return antenna::get(std::span(antennas_), id); }

    bool Setup::isUpToDate(std::filesystem::path const& path_timestamp) const
    {
        timeutil::timestamp_t const saved_timestamp = std::filesystem::exists(path_timestamp) ? timeutil::load_from_file(path_timestamp) : 0;

        // we skip if the timestamps match and are non-zero
        // zero timestamps are used by the testing framework to force setup's tasks execution
        return saved_timestamp && saved_timestamp == timestamp_;
    }

    double Setup::get_double(std::string const& variable_name) const
    {
        auto const var = variables_.at(variable_name);
        auto const ptr = std::get_if<double>(&var);
        if (!ptr) { throw SimulationError("Variable '{}' is not a double", variable_name); }
        return *ptr;
    }

    std::int64_t Setup::get_int(std::string const& variable_name) const
    {
        auto const var = variables_.at(variable_name);
        auto const ptr = std::get_if<std::int64_t>(&var);
        if (!ptr) { throw SimulationError("Variable '{}' is not an int", variable_name); }
        return *ptr;
    }

    void Setup::extract_meta(ojson& js)
    {
        auto& metadata = js.at("metadata");
        name_ = factory::get_string(metadata, "setup_name");
        std::println("Setup name: {}", name_);
        factory::assert_empty(metadata);
        js.erase("metadata");
    }

    void Setup::extract_codebooks(ojson& js)
    {
        if (js.contains("codebooks"))
        {
            auto const& desc = js.at("codebooks");
            try
            {
                for (auto& cb_desc : desc)
                {
                    auto id = cb_desc.at("id").get<std::string>();
                    auto path = cb_desc.at("path").get<std::string>();
                    codebooks_.emplace_back(id, path);
                }
                js.erase("codebooks");
            }
            catch (...)
            {
                std::throw_with_nested(SimulationError("Failed to parse codebooks:\n{}'", desc.dump(2)));
            }
        }
    }

    void Setup::extract_num_params(ojson& js)
    {
        if (js.contains("num_params"))
        {
            auto const& desc = js.at("num_params");
            try
            {
                desc.get_to(num_params_);
                js.erase("num_params");
            }
            catch (...)
            {
                std::throw_with_nested(SimulationError("Failed to parse numerical parameters:\n{}'", desc.dump(2)));
            }
        }
    }

    void Setup::extract_variables(ojson& js)
    {
        // we always provide the system wavelength as a variable for convenience
        variables_["system_wavelength"] = num_params_.system_wavelength;

        if (!js.contains("variables")) { return; }
        for (auto& variables = js.at("variables"); const auto& [raw_key, val] : variables.items())
        {
            try
            {
                auto const colon_pos = raw_key.find(':');
                auto const stripped_key = std::string_view(raw_key).substr(0, colon_pos);
                auto const type = colon_pos == std::string_view::npos ? std::string_view() : stripped_key.substr(colon_pos + 1);
                bool is_double = type.empty() or type == "double";
                if (!is_double and type != "int") { throw SimulationError("Variable '{}' has invalid type specifier '{}'", stripped_key, type); }
                std::string key(stripped_key);

                if (variables_.contains(key)) throw SimulationError("Variable '{}' already exists", key);

                if (val.is_string())
                {
                    if (is_double) { variables_[key] = factory::parse_double(val.get<std::string>(), variables_); }
                    else
                    {
                        variables_[key] = factory::parse_int(val.get<std::string>(), variables_);
                    }
                }
                else if (val.is_number())
                {
                    if (is_double) { variables_[key] = val.is_number_float() ? val.get<double>() : static_cast<double>(val.get<std::int64_t>()); }
                    else
                    {
                        variables_[key] = val.is_number_float() ? static_cast<std::int64_t>(std::roundl(val.get<long double>())) : val.get<std::int64_t>();
                    }
                }
                else
                {
                    throw SimulationError("Invalid type '{}' of variable '{}'", val.type_name(), stripped_key);
                }
                std::visit(
                    [&key](const auto& var)
                    {
                        if constexpr (std::is_same_v<std::decay_t<decltype(var)>, std::int64_t>)
                            std::println("Define integer variable {}={}", key, var);
                        else
                            std::println("Define floating-point variable {}={:.15g}", key, var);
                    },
                    variables_.at(key));
            }
            catch (...)
            {
                std::throw_with_nested(SimulationError("Failed to process variable '{}'", raw_key));
            }
        }
        js.erase("variables");
    }

    void Setup::extract_references(ojson& js)
    {
        references_.emplace_back(); // dummy reference to global origin
        if (!js.contains("references")) { return; }
        references_.reserve(references_.size() + js.at("references").size());
        for (auto& references = js.at("references"); auto& desc : references) { references_.push_back(factory::make_reference(desc, variables_)); }
        js.erase("references");
    }

    void Setup::extract_antennas(ojson& js)
    {
        if (!js.contains("antennas")) { return; }
        for (auto& antennas = js.at("antennas"); auto& desc : antennas) antennas_.push_back(factory::make_antenna(desc, variables_));
        js.erase("antennas");
    }

    void Setup::extract_geometries(ojson& js)
    {
        if (!js.contains("geometries")) { return; }
        for (auto& geometries = js.at("geometries"); auto& desc : geometries) geometries_.push_back(factory::make_geometry(desc, variables_));
        js.erase("geometries");
    }

    void Setup::extract_sweeps(ojson& js)
    {
        if (!js.contains("sweeps")) { return; }
        for (auto& sweeps = js.at("sweeps"); auto& desc : sweeps) sweeps_.push_back(factory::make_sweep(desc, variables_));
        js.erase("sweeps");
    }

    void Setup::extract_tasks(ojson& js)
    {
        if (!js.contains("tasks")) { return; }
        for (auto& tasks = js.at("tasks"); auto& desc : tasks) tasks_.push_back(factory::make_task(desc, variables_));
        js.erase("tasks");
    }
} // namespace setup
