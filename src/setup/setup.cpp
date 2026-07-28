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
#include "manifest.hpp"
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
    } // namespace

    Setup::Setup(std::filesystem::path const& path_json, bool override_timestamp) : Setup(load_json(path_json))
    { timestamp_ = timeutil::get_of_file(path_json); }

    Setup::Setup(ojson const& js_in)
    {
        ojson js = js_in; // create a copy of the json object in order to decompose it
        extract_meta(js);
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

        for (auto const& geo : geometries_)
        {
            std::visit(
                [&container](auto const& g)
                {
                    if constexpr (std::constructible_from<geometry::Curve, decltype(g)>)
                    {
                        auto curve = geometry::Curve(g);

                        if constexpr (std::is_same_v<geometry::CircleArc, std::decay_t<decltype(g)>>)
                        {
                            container.add(three::create_coordinate_arrows( //
                                g.center(),
                                g.e1(),
                                g.e2(),
                                g.normal(),
                                0.25 * g.length() //
                                ));
                        }
                        std::vector<Pos> points(N_POINTS_THREE_GEOMETRIES);
                        for (std::size_t k = 0; k < N_POINTS_THREE_GEOMETRIES; k++)
                        {
                            double t = static_cast<double>(k) / static_cast<double>(N_POINTS_THREE_GEOMETRIES - 1);
                            points.at(k) = geometry::curve::get_pos_at(curve, t);
                        }
                        container.add(three::make_line(points, 2.0, Color::yellow));
                    }
                    else if constexpr (std::constructible_from<geometry::Surface, decltype(g)>)
                    {
                        auto surface = geometry::Surface(g);
                        container.add(three::create_coordinate_arrows( //
                            geometry::surface::get_center(g),
                            geometry::surface::get_e1(g),
                            geometry::surface::get_e2(g),
                            geometry::surface::get_normal(g),
                            0.25 * std::sqrt(geometry::surface::get_area(g)) //
                            ));
                        for (std::size_t k1 = 0; k1 < N_POINTS_THREE_GEOMETRIES; k1++)
                        {
                            double const t1 = static_cast<double>(k1) / static_cast<double>(N_POINTS_THREE_GEOMETRIES - 1);
                            std::vector<Pos> points(N_POINTS_THREE_GEOMETRIES);
                            for (std::size_t k2 = 0; k2 < N_POINTS_THREE_GEOMETRIES; k2++)
                            {
                                double const t2 = static_cast<double>(k2) / static_cast<double>(N_POINTS_THREE_GEOMETRIES - 1);
                                points.at(k2) = geometry::surface::get_pos_at(surface, t1, t2);
                            }
                            container.add(three::make_line(points, 2.0, Color::yellow));
                        }

                        for (std::size_t k2 = 0; k2 < N_POINTS_THREE_GEOMETRIES; k2++)
                        {
                            double const t2 = static_cast<double>(k2) / static_cast<double>(N_POINTS_THREE_GEOMETRIES - 1);
                            std::vector<Pos> points(N_POINTS_THREE_GEOMETRIES);
                            for (std::size_t k1 = 0; k1 < N_POINTS_THREE_GEOMETRIES; k1++)
                            {
                                double const t1 = static_cast<double>(k1) / static_cast<double>(N_POINTS_THREE_GEOMETRIES - 1);
                                points.at(k1) = geometry::surface::get_pos_at(surface, t1, t2);
                            }
                            container.add(three::make_line(points, 2.0, Color::yellow));
                        }
                    }
                    else
                        throw SimulationError("Invalid geometry object");
                },
                geo);
        }
        container.export_to_javascript(p);
    }

    void Setup::run_tasks(std::filesystem::path const& path_cwd)
    {
        for (auto const& task : tasks_)
        {
            std::filesystem::path const path_json = path_cwd / (setup::task::get_id(task) + ".result.json");
            if (std::holds_alternative<task::DirectivityOverPolarSweepAzimuth>(task))
            {
                auto& t = std::get<task::DirectivityOverPolarSweepAzimuth>(task);
                eval::output::directivity_over_polar( //
                    path_json,
                    antenna::get(antennas_, t.antenna_id),
                    sweep::get(sweeps_, t.sweep_azimuth_id),
                    num_params_ //
                );
            }
            else if (std::holds_alternative<task::RxVoltageFieldSweepWavelength>(task))
            {
                auto& t = std::get<task::RxVoltageFieldSweepWavelength>(task);
                eval::output::voltagefield_over_geometry( //
                    path_json,
                    RxVoltageField(antenna::get(antennas_, t.tx_id), antenna::get(antennas_, t.rx_id), num_params_),
                    geometry::get(geometries_, t.geo_id),
                    sweep::get(sweeps_, t.sweep_wavelength_id) //
                );
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
                        if constexpr (std::is_same_v<std::decay_t<decltype(var)>, std::int64_t>) { std::println("Define integer variable {}={}", key, var); }
                        else
                        {
                            std::println("Define floating-point variable {}={:.15g}", key, var);
                        }
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
        for (auto& antennas = js.at("antennas"); auto& desc : antennas) { antennas_.push_back(factory::make_antenna(desc, variables_)); }
        js.erase("antennas");
    }

    void Setup::extract_geometries(ojson& js)
    {
        if (!js.contains("geometries")) { return; }
        for (auto& geometries = js.at("geometries"); auto& desc : geometries) { geometries_.push_back(factory::make_geometry(desc, variables_)); }
        js.erase("geometries");
    }

    void Setup::extract_sweeps(ojson& js)
    {
        if (!js.contains("sweeps")) { return; }
        for (auto& sweeps = js.at("sweeps"); auto& desc : sweeps) { sweeps_.push_back(factory::make_sweep(desc, variables_)); }
        js.erase("sweeps");
    }

    void Setup::extract_tasks(ojson& desc)
    {
        if (!desc.contains("tasks")) { return; }
        for (auto& tasks = desc.at("tasks"); auto& task_desc : tasks)
        {
            auto const type = factory::get_string(task_desc, "type");
            std::println("Found task of type '{}'", type);
            task::Task task;
            if (type == "DirectivityOverPolar@Azimuth")
            {
                auto antenna_id = factory::get_string(task_desc, "antenna");
                auto sweep_azimuth_id = factory::get_string(task_desc, "sweep_azimuth");
                task = task::DirectivityOverPolarSweepAzimuth{{}, antenna_id, sweep_azimuth_id};
            }
            else if (type == "RxVoltageField@Wavelength")
            {
                auto tx_id = factory::get_string(task_desc, "tx");
                auto rx_id = factory::get_string(task_desc, "rx");
                auto geo_id = factory::get_string(task_desc, "geo");
                auto sweep_wavelength_id = factory::get_string(task_desc, "sweep_wavelength");
                task = task::RxVoltageFieldSweepWavelength{{}, tx_id, rx_id, geo_id, sweep_wavelength_id};
            }
            else
                throw SimulationError("Unknown task type \"{}\"", type);
            tasks_.push_back(task);
            std::println("Created task: {}", setup::task::get_id(task));
            factory::assert_empty(task_desc);
        }
        desc.erase("tasks");
    }
} // namespace setup
