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
    template <typename ContainerType>
    ContainerType& json_get(ContainerType& js, std::string_view key)
    {
        factory::assert_key(js, key);
        return js[key];
    }

    void extract_variables(factory::Context& context)
    {
        if (!context.desc.contains("variables")) { return; }
        for (auto& variables = context.desc.at("variables"); const auto& [key, val] : variables.items())
        {
            if (val.is_string()) { context.variables[key] = factory::parse_double(val.get<std::string>(), context.variables); }
            else if (val.is_number()) { context.variables[key] = val.get<double>(); }
            else
            {
                throw SimulationError("Invalid type '{}' of variable '{}'", val.type_name(), key);
            }
            std::println("Define variable {}={:.15g}", key, context.variables[key]);
        }
        context.desc.erase("variables");
    }

    void extract_references(factory::Context& context)
    {
        context.references.emplace_back("", nullptr, pos_t(0, 0, 0), Quaternion(0, 0, 0)); // dummy reference to global origin
        if (!context.desc.contains("references")) { return; }
        for (auto& references = context.desc.at("references"); auto& desc : references)
        {
            factory::make_reference(desc, context.references, context.variables);
            factory::assert_empty(desc);
        }
        context.desc.erase("references");
    }

    void extract_antennas(factory::Context& context)
    {
        if (!context.desc.contains("antennas")) { return; }
        for (auto& antennas = context.desc.at("antennas"); auto& desc : antennas)
        {
            Antenna ant = factory::make_antenna(desc, context);
            context.antennas.emplace(antenna::get_id(ant), std::move(ant));
            factory::assert_empty(desc);
        }
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
                auto const azimuth_angles = factory::get_ndarray(task_desc, "azimuth_angles") * nc::constants::pi;
                Antenna const& ant = context.antennas.at(factory::get_string(task_desc, "antenna"));
                task_name = std::format("{}.{}", type, antenna::get_id(ant));
                context.tasks.emplace_back(task_name, [&ant, azimuth_angles] { plot::plot_directivity_over_polar(ant, azimuth_angles, {}); });
            }
            else if (type == "plot_gain_over_straight")
            {
                Antenna const& tx = context.antennas.at(factory::get_string(task_desc, "tx"));
                Antenna& rx = context.antennas.at(factory::get_string(task_desc, "rx"));
                Reference& ref_start = factory::find_reference_by_id(context.references, factory::get_string(task_desc, "ref_start"));
                Reference const& ref_stop = factory::find_reference_by_id(context.references, factory::get_string(task_desc, "ref_stop"));
                double wavelength = factory::get_double(task_desc, "wavelength", context.variables);
                math::NumParams num_params{.wavelength = wavelength};
                pos_t const pos_start = ref_start.global_pos();
                pos_t const pos_end = ref_stop.global_pos();
                auto power_field = antenna::get_voltage_field(tx, rx, num_params);
                task_name = std::format("{}.{}.{}", type, antenna::get_id(tx), antenna::get_id(rx));
                context.tasks.emplace_back(task_name, [power_field, pos_start, pos_end] { plot::plot_gain_over_line(power_field, pos_start, pos_end); });
            }
            else if (type == "plot_voltage_field")
            {
                auto const tx_id = factory::get_string(task_desc, "tx");
                Antenna const& tx = context.antennas.at(tx_id);
                Antenna& rx = context.antennas.at(factory::get_string(task_desc, "rx"));
                auto geo = context.geometries.at(factory::get_string(task_desc, "geometry"));
                auto voltage_field = antenna::get_voltage_field(tx, rx, context.num_params);
                task_name = std::format("{}.{}.{}", type, tx_id, antenna::get_id(rx));
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
    auto& metadata = json_get(setup_desc, "metadata");
    auto const setup_name = factory::get_string(metadata, "setup_name");
    std::println("Setup name: {}", setup_name);
    factory::assert_empty(metadata);
    setup_desc.erase("metadata");

    factory::Context context{.desc = setup_desc};
    if (setup_desc.contains("num_params")) { setup_desc.at("num_params").get_to(context.num_params); setup_desc.erase("num_params");}
    else
    {
        std::println("{}No numerical parameters specified, using default configuration{}", ansi_color::fg4::bright_yellow, ansi_color::reset);
    }

    extract_variables(context);
    extract_references(context);
    extract_antennas(context);
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

    if (!variables.contains("system_wavelength")) { throw SimulationError("Missing mandatory variable 'system_wavelength' for three export"); }
    double const system_wavelength = variables.at("system_wavelength");

    three::Container container;
    for (auto const& reference : references)
    {
        if (!reference.origin) { continue; } // we skip the dummy reference
        auto const pos_center = reference.global_from_local_pos(POS_ZERO);
        auto const pos_origin = reference.origin->global_from_local_pos(POS_ZERO);
        // auto const distance = (pos_center - pos_origin).norm();
        container.add(three::make_line(pos_origin, pos_center, 1.0, Color::white));
        container.add(three::create_coordinate_arrows(pos_center, reference.global_from_local_pos({1, 0, 0}) - pos_center,
                                                      reference.global_from_local_pos({0, 1, 0}) - pos_center,
                                                      reference.global_from_local_pos({0, 0, 1}) - pos_center, 0.25 * system_wavelength));
    }

    auto add_radiator = [&container, &system_wavelength](Radiator const& radiator)
    {
        auto pos_center = radiator.origin.global_from_local_pos(POS_ZERO);
        double const radiator_length = 0.49 * system_wavelength;
        auto pos_end = radiator.origin.global_from_local_pos({0.0, 0.0, 0.5 * radiator_length});
        auto const pos_start = pos_center - (pos_end - pos_center);
        double const radius = 0.1 * radiator_length;
        container.add(three::make_cylinder(pos_start, pos_end, radius, radius));
    };
    for (const auto& antenna : antennas | std::views::values)
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
    for (auto& [task_name, task] : tasks)
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

Reference& Setup::get_reference(std::string_view const id) { return factory::find_reference_by_id(references, id); }

Antenna& Setup::get_antenna(std::string const& id) { return antennas.at(id); }

bool Setup::isUpToDate(std::filesystem::path const& path_timestamp) const
{
    timeutil::timestamp_t const saved_timestamp = std::filesystem::exists(path_timestamp) ? timeutil::load_from_file(path_timestamp) : 0;

    // we skip if the timestamps match and are non-zero
    // zero timestamps are used by the testing framework to force setup's tasks execution
    return saved_timestamp && saved_timestamp == timestamp;
}

Setup::Setup(std::string_view const name, timeutil::timestamp_t const timestamp, factory::Context&& context) :
    name(name), timestamp(timestamp), variables(std::move(context.variables)), references(std::move(context.references)), antennas(std::move(context.antennas)),
    tasks(std::move(context.tasks))
{}
