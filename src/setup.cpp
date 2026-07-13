//
// Created by Tristan Krause on 2026-05-26.
//

#include "setup.hpp"
#include <algorithm>
#include <fstream>
#include <memory>
#include <print>
#include <ansi_color.hpp>
#include <nlohmann/json.hpp>
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
        for (auto& variables = context.desc["variables"]; const auto& [key, val] : variables.items())
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
        for (auto& references = json_get(context.desc, "references"); auto& reference_desc : references)
        {
            factory::make_reference(reference_desc, context.references, context.variables);
            factory::assert_empty(reference_desc);
        }
        context.desc.erase("references");
    }

    void extract_antennas(factory::Context& context)
    {
        if (!context.desc.contains("antennas")) { return; }
        for (auto& antennas = json_get(context.desc, "antennas"); auto& desc : antennas)
        {
            Antenna ant = factory::make_antenna(desc, context);
            context.antennas.emplace(antenna::get_id(ant), std::move(ant));
            factory::assert_empty(desc);
        }
        context.desc.erase("antennas");
    }

    void extract_tasks(factory::Context& context)
    {
        if (!context.desc.contains("tasks")) { return; }
        for (auto& tasks = json_get(context.desc, "tasks"); auto& task_desc : tasks)
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
                context.tasks.emplace_back(task_name, [&ant, azimuth_angles](std::filesystem::path const& directory)
                                           { plot::plot_directivity_over_polar(directory, ant, azimuth_angles); });
            }
            else if (type == "plot_gain_over_straight")
            {
                Antenna const& tx = context.antennas.at(factory::get_string(task_desc, "tx"));
                Antenna const& rx = context.antennas.at(factory::get_string(task_desc, "rx"));
                Reference& ref_start = factory::find_reference_by_id(context.references, factory::get_string(task_desc, "ref_start"));
                Reference const& ref_stop = factory::find_reference_by_id(context.references, factory::get_string(task_desc, "ref_stop"));
                double wavelength = factory::get_double(task_desc, "wavelength", context.variables);
                char distance_axis = factory::get_char(task_desc, "distance_axis");
                task_name = std::format("{}.{}.{}", type, antenna::get_id(tx), antenna::get_id(rx));
                context.tasks.emplace_back(task_name, [&tx, &rx, &ref_start, &ref_stop, wavelength, distance_axis](std::filesystem::path const& directory)
                                           { plot::plot_gain_over_straight(directory, tx, rx, ref_start, ref_stop, wavelength, distance_axis); });
            }
            else if (type == "plot_gain_over_plane")
            {
                auto const tx_id = factory::get_string(task_desc, "tx");
                Antenna const& tx = context.antennas.at(tx_id);
                Antenna & rx = context.antennas.at(factory::get_string(task_desc, "rx"));
                Reference& ref_zero = factory::find_reference_by_id(context.references, factory::get_string(task_desc, "ref_zero"));
                Reference const& ref_axis1_max = factory::find_reference_by_id(context.references, factory::get_string(task_desc, "ref_axis1_max"));
                Reference const& ref_axis2_max = factory::find_reference_by_id(context.references, factory::get_string(task_desc, "ref_axis2_max"));
                double wavelength = factory::get_double(task_desc, "wavelength", context.variables);
                std::uint32_t n_points_axis1 = factory::get_uint(task_desc, "n_points_axis1", context.variables);
                std::uint32_t n_points_axis2 = factory::get_uint(task_desc, "n_points_axis2", context.variables);
                auto label_axis1 = factory::get_string(task_desc, "label_axis1");
                auto label_axis2 = factory::get_string(task_desc, "label_axis2");

                pos_t const pos_zero = ref_zero.global_from_local_pos(POS_ZERO);
                pos_t const pos_axis1_max = ref_axis1_max.global_from_local_pos(POS_ZERO);
                pos_t const pos_axis2_max = ref_axis2_max.global_from_local_pos(POS_ZERO);
                math::Rectangle rectangle = math::get_rectangle(pos_zero, pos_axis1_max, pos_axis2_max);

                auto voltage_field = antenna::get_voltage_field(tx, rx, {});
                task_name = std::format("{}.{}.{}", type, tx_id, antenna::get_id(rx));
                context.tasks.emplace_back(task_name,
                                           [&tx, &rx, voltage_field, rectangle, wavelength, n_points_axis1, n_points_axis2, label_axis1, label_axis2](std::filesystem::path const& directory)
                                           {
                                               plot::plot_gain_over_plane(directory, voltage_field, rectangle, wavelength, n_points_axis1, n_points_axis2, label_axis1, label_axis2);
                                           });
            }
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
    extract_variables(context);
    extract_references(context);
    extract_antennas(context);
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
            task(std::filesystem::current_path());
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
