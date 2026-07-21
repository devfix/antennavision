//
// Created by Tristan Krause on 2026-05-26.
//

#include "setup.hpp"
#include <algorithm>
#include <ansi_color.hpp>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <print>
#include "factory/find.hpp"
#include "factory/get.hpp"
#include "factory/make.hpp"
#include "factory/parse.hpp"
#include "plot.hpp"
#include "simulationerror.hpp"
#include "three.hpp"

using namespace ansi_color;

namespace
{
    void extract_variables(factory::Context& context)
    {
        if (!context.desc.contains("variables")) { return; }
        for (auto& variables = context.desc.at("variables"); const auto& [raw_key, val] : variables.items())
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
                    if (is_double) { context.variables[key] = factory::parse_double(val.get<std::string>(), context.variables); }
                    else
                    {
                        context.variables[key] = factory::parse_int(val.get<std::string>(), context.variables);
                    }
                }
                else if (val.is_number())
                {
                    if (is_double) { context.variables[key] = val.is_number_float() ? val.get<double>() : static_cast<double>(val.get<std::int64_t>()); }
                    else
                    {
                        context.variables[key] =
                            val.is_number_float() ? static_cast<std::int64_t>(std::roundl(val.get<long double>())) : val.get<std::int64_t>();
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
                    context.variables.at(key));
            }
            catch (...)
            {
                std::throw_with_nested(SimulationError("Failed to process variable '{}'", raw_key));
            }
        }
        context.desc.erase("variables");
    }

    void extract_references(factory::Context& context)
    {
        context.references.emplace_back(); // dummy reference to global origin
        if (!context.desc.contains("references")) { return; }
        context.references.reserve(1 + context.desc.at("references").size());
        for (auto& references = context.desc.at("references"); auto& desc : references)
        {
            context.references.push_back(factory::make_reference(desc, context));
        }
        context.desc.erase("references");
    }

    void extract_antennas(factory::Context& context)
    {
        if (!context.desc.contains("antennas")) { return; }
        for (auto& antennas = context.desc.at("antennas"); auto& desc : antennas) { context.antennas.push_back(factory::make_antenna(desc, context)); }
        context.desc.erase("antennas");
    }

    void extract_geometries(factory::Context& context)
    {
        if (!context.desc.contains("geometries")) { return; }
        for (auto& geometries = context.desc.at("geometries"); auto& desc : geometries)
        {
            auto id = factory::get_string(desc, "id", false, false);
            Geometry geo = factory::make_geometry(desc, context);
            context.geometries.emplace(id, std::move(geo));
            factory::assert_empty(desc);
        }
        context.desc.erase("geometries");
    }

    void extract_tasks(factory::Context& context)
    {
        if (!context.desc.contains("tasks")) { return; }
        for (auto& tasks = context.desc.at("tasks"); auto& task_desc : tasks)
        {
            auto const type = factory::get_string(task_desc, "type");
            std::println("Found task of type '{}'", type);
            std::string task_name;
            if (type == "builtin")
            {
                auto const key = factory::get_string(task_desc, "key");
                task_name = std::format("builtin.{}", key);
                context.tasks.emplace_back(task_name, nullptr);
            }
            else if (type == "plot_directivity_over_polar")
            {
                auto azimuth_angles_vec = task_desc.at("azimuth_angles").get<std::vector<double>>();
                auto azimuth_angles = RealArray(azimuth_angles_vec.size(), 1);
                std::ranges::transform(azimuth_angles_vec, azimuth_angles.begin(), [](auto const v) { return v * nc::constants::pi; });
                Antenna const& ant = antenna::get(context.antennas, factory::get_string(task_desc, "antenna"));
                task_name = std::format("{}.{}", type, antenna::get_id(ant));
                context.tasks.emplace_back(task_name, [&ant, azimuth_angles] { plot::plot_directivity_over_polar(ant, azimuth_angles, {}); });
            }
            else if (type == "plot_gain_over_straight")
            {
                Antenna const& tx = antenna::get(context.antennas, factory::get_string(task_desc, "tx"));
                Antenna& rx = antenna::get(context.antennas, factory::get_string(task_desc, "rx"));
                Reference& ref_start = factory::find_reference_by_id(context.references, factory::get_string(task_desc, "ref_start"));
                Reference const& ref_stop = factory::find_reference_by_id(context.references, factory::get_string(task_desc, "ref_stop"));
                double wavelength = factory::get_double(task_desc, "wavelength", context.variables);
                math::NumParams num_params{.system_wavelength = wavelength};
                pos_t const pos_start = ref_start.global_pos();
                pos_t const pos_end = ref_stop.global_pos();
                auto power_field = antenna::get_voltage_field(tx, rx, num_params);
                task_name = std::format("{}.{}.{}", type, antenna::get_id(tx), antenna::get_id(rx));
                context.tasks.emplace_back(task_name, [power_field, pos_start, pos_end] { plot::plot_gain_over_line(power_field, pos_start, pos_end); });
            }
            else if (type == "plot_voltage_field")
            {
                Antenna const& tx = antenna::get(context.antennas, factory::get_string(task_desc, "tx"));
                Antenna& rx = antenna::get(context.antennas, factory::get_string(task_desc, "rx"));
                auto geo = context.geometries.at(factory::get_string(task_desc, "geometry"));
                auto voltage_field = antenna::get_voltage_field(tx, rx, context.num_params);
                task_name = std::format("{}.{}.{}", type, antenna::get_id(tx), antenna::get_id(rx));
                context.tasks.emplace_back(task_name, [voltage_field, geo] { plot::plot_field_over_geometry(voltage_field, geo); });
            }
            // else if (type == "plot_gain_over_sphere")
            // {
            //     auto const tx_id = factory::get_string(task_desc, "tx");
            //     Antenna const& tx = context.antennas.at(tx_id);
            //     Antenna& rx = context.antennas.at(factory::get_string(task_desc, "rx"));
            //     Reference& ref_center = factory::find_reference_by_id(context.references, factory::get_string(task_desc, "ref_center"));
            //     Reference& ref_rect = factory::find_reference_by_id(context.references, factory::get_string(task_desc, "ref_rect"));
            //     double wavelength = factory::get_double(task_desc, "wavelength", context.variables);
            //     double const polar = factory::get_double(task_desc, "polar", context.variables);
            //     double const azimuth = factory::get_double(task_desc, "azimuth", context.variables);
            //     std::uint32_t const n_points_polar = factory::get_uint(task_desc, "n_points_polar", context.variables);
            //     std::uint32_t const n_points_azimuth = factory::get_uint(task_desc, "n_points_azimuth", context.variables);
            //     auto const dir_north = factory::get_pos(task_desc, "dir_north", context.variables);
            //     pos_t const center = ref_center.global_pos();
            //     pos_t const pos_rect = ref_rect.global_pos();
            //     auto sr = geometry::SphericalRectangle::make(center, pos_rect, polar * pi, azimuth * pi, dir_north);
            //     math::NumParams num_params{.wavelength = wavelength, .n_polar = n_points_polar, .n_azimuth = n_points_azimuth};
            //     auto voltage_field = antenna::get_voltage_field(tx, rx, num_params);
            //     task_name = std::format("{}.{}.{}", type, tx_id, antenna::get_id(rx));
            //     context.tasks.emplace_back(task_name, [voltage_field, sr] { plot::plot_gain_over_spherical_rectangle(voltage_field, sr); });
            // }
            else
            {
                throw SimulationError("Unknown task type \"{}\"", type);
            }
            std::println("Created task: {}", task_name);
            factory::assert_empty(task_desc);
        }
        context.desc.erase("tasks");
    }
} // namespace

std::unique_ptr<Setup> Setup::from_json(nlohmann::ordered_json const& js, timeutil::timestamp_t const timestamp)
{
    ojson setup_desc = js; // create a copy of the json object in order to decompose it
    auto& metadata = setup_desc.at("metadata");
    auto const setup_name = factory::get_string(metadata, "setup_name");
    std::println("Setup name: {}", setup_name);
    factory::assert_empty(metadata);
    setup_desc.erase("metadata");

    factory::Context context{.desc = setup_desc};
    if (setup_desc.contains("num_params"))
    {
        setup_desc.at("num_params").get_to(context.num_params);
        setup_desc.erase("num_params");
    }
    else
    {
        std::println("{}No numerical parameters specified, using default configuration{}", ansi_color::fg4::bright_yellow, ansi_color::reset);
    }

    extract_variables(context);
    extract_references(context);
    extract_antennas(context);
    Reference::resolve_origins(context.references);
    antenna::resolve_origins(context.antennas, context.references);
    extract_geometries(context);
    extract_tasks(context);
    factory::assert_empty(context.desc);

    // ReSharper disable once CppDFAMemoryLeak
    return std::unique_ptr<Setup>(new Setup(setup_name, timestamp, std::move(context)));
}

std::unique_ptr<Setup> Setup::from_file(std::filesystem::path const& p)
{
    std::println("{}Loading setup file '{}'{}", ansi_color::fg4::cyan, p.string(), ansi_color::reset);
    std::ifstream file(p);
    if (!file.is_open()) { throw SimulationError("Could not open setup file '{}'", p.string()); }
    auto const js = nlohmann::ordered_json::parse(file);
    file.close();
    return from_json(js, timeutil::get_of_file(p));
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
        container.add(three::create_coordinate_arrows(pos_center, reference.global_from_local_pos({1, 0, 0}) - pos_center,
            reference.global_from_local_pos({0, 1, 0}) - pos_center, reference.global_from_local_pos({0, 0, 1}) - pos_center, 0.25 * system_wavelength));
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

Antenna& Setup::get_antenna(std::string const& id) { return antenna::get(antennas_, id); }

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

Setup::Setup(std::string_view const name, timeutil::timestamp_t const timestamp, factory::Context&& context) :
    name_(name), timestamp_(timestamp), variables_(std::move(context.variables)), num_params_(context.num_params), references_(std::move(context.references)),
    antennas_(std::move(context.antennas)), tasks_(std::move(context.tasks))
{}
