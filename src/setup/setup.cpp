//
// Created by Tristan Krause on 2026-05-26.
//

#include "setup/setup.hpp"
#include <algorithm>
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
{
    timestamp_ = timeutil::get_of_file(path_json);
}

Setup::Setup(ojson const& js_in)
{
    ojson js = js_in; // create a copy of the json object in order to decompose it
    extract_meta(js);
    extract_num_params(js);
    extract_variables(js);
    extract_references(js);
    extract_antennas(js);
    extract_geometries(js);
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
    for (const auto& antenna : antennas_)
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
    container.export_to_javascript(p);
}

void Setup::run_tasks(std::function<void(std::string_view)> const& builtin_handler)
{
    for (auto& [task_name, task] : tasks_)
    {
        if (task_name.starts_with("builtin."))
        {
            std::println("Running builtin task: {}", task_name);
            builtin_handler(task_name.substr(std::strlen("builtin.")));
        }
        else
        {
            std::println("Running task: {}", task_name);
            task();
        }
    }
}

Reference& Setup::get_reference(std::string_view const id) { return factory::find_reference_by_id(references_, id); }

antenna::Antenna& Setup::get_antenna(std::string const& id)
{
    return antenna::get(std::span(antennas_), id);
}

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

void Setup::extract_tasks(ojson& desc)
{
    if (!desc.contains("tasks")) { return; }
    for (auto& tasks = desc.at("tasks"); auto& task_desc : tasks)
    {
        auto const type = factory::get_string(task_desc, "type");
        std::println("Found task of type '{}'", type);
        std::string task_name;
        if (type == "builtin")
        {
            auto const key = factory::get_string(task_desc, "key");
            task_name = std::format("builtin.{}", key);
            tasks_[task_name] = nullptr;
        }
        else if (type == "plot_directivity_over_polar")
        {
            auto azimuth_angles_vec = task_desc.at("azimuth_angles").get<std::vector<double>>();
            auto azimuth_angles = RealArray(azimuth_angles_vec.size(), 1);
            std::ranges::transform(azimuth_angles_vec, azimuth_angles.begin(), [](auto const v) { return v * nc::constants::pi; });
            antenna::Antenna const& ant = antenna::get(antennas_, factory::get_string(task_desc, "antenna"));
            task_name = std::format("{}.{}", type, antenna::get_id(ant));
            tasks_[task_name] = [&ant, azimuth_angles] { eval::output::directivity_over_polar(ant, azimuth_angles, {}); };
        }
        else if (type == "compute_voltage_field")
        {
            antenna::Antenna const& tx = antenna::get(antennas_, factory::get_string(task_desc, "tx"));
            antenna::Antenna& rx = antenna::get(antennas_, factory::get_string(task_desc, "rx"));
            auto geo = geometry::get(geometries_, factory::get_string(task_desc, "geometry"));
            VoltageField voltage_field(tx, rx, num_params_);
            task_name = std::format("{}.{}.{}", type, antenna::get_id(tx), antenna::get_id(rx));
            tasks_[task_name] = [voltage_field, geo] { eval::output::voltagefield_over_geometry(voltage_field, geo); };
        }
        // else if (type == "plot_gain_over_sphere")
        // {
        //     auto const tx_id = factory::get_string(task_desc, "tx");
        //     Antenna const& tx = antennas.at(tx_id);
        //     Antenna& rx = antennas.at(factory::get_string(task_desc, "rx"));
        //     Reference& ref_center = factory::find_reference_by_id(references, factory::get_string(task_desc, "ref_center"));
        //     Reference& ref_rect = factory::find_reference_by_id(references, factory::get_string(task_desc, "ref_rect"));
        //     double wavelength = factory::get_double(task_desc, "wavelength", variables);
        //     double const polar = factory::get_double(task_desc, "polar", variables);
        //     double const azimuth = factory::get_double(task_desc, "azimuth", variables);
        //     std::uint32_t const n_points_polar = factory::get_uint(task_desc, "n_points_polar", variables);
        //     std::uint32_t const n_points_azimuth = factory::get_uint(task_desc, "n_points_azimuth", variables);
        //     auto const dir_north = factory::get_pos(task_desc, "dir_north", variables);
        //     pos_t const center = ref_center.global_pos();
        //     pos_t const pos_rect = ref_rect.global_pos();
        //     auto sr = geometry::SphericalRectangle::make(center, pos_rect, polar * pi, azimuth * pi, dir_north);
        //     setup::NumParams num_params{.wavelength = wavelength, .n_polar = n_points_polar, .n_azimuth = n_points_azimuth};
        //     auto voltage_field = antenna::get_voltage_field(tx, rx, num_params);
        //     task_name = std::format("{}.{}.{}", type, tx_id, antenna::get_id(rx));
        //     tasks.emplace_back(task_name, [voltage_field, sr] { plot::plot_gain_over_spherical_rectangle(voltage_field, sr); });
        // }
        else
        {
            throw SimulationError("Unknown task type \"{}\"", type);
        }
        std::println("Created task: {}", task_name);
        factory::assert_empty(task_desc);
    }
    desc.erase("tasks");
}
