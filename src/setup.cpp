//
// Created by Tristan Krause on 2026-05-26.
//

#include "setup.hpp"
#include <algorithm>
#include <ansi_color.hpp>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include "factory/find.hpp"
#include "factory/get.hpp"
#include "factory/make.hpp"
#include "factory/parse.hpp"
#include "plot.hpp"
#include "print.hpp"
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

} // namespace

std::unique_ptr<Setup> Setup::from_json(nlohmann::ordered_json const& js, timeutil::timestamp_t const timestamp)
{
    json setup_desc = js; // create a copy of the json object in order to decompose it
    auto const& metadata = json_get(setup_desc, "metadata");
    std::string_view const setup_name = json_get(metadata, "setup_name").get<std::string_view>();
    std::println("Setup name: {}", setup_name);

    std::map<std::string, double> variables;
    if (setup_desc.contains("variables"))
    {
        for (const auto& [key, val] : setup_desc["variables"].items())
        {
            if (val.is_string()) { variables[key] = factory::parse_double(val.get<std::string>(), variables); }
            else if (val.is_number()) { variables[key] = val.get<double>(); }
            else
            {
                throw SimulationError("Invalid type '{}' of variable '{}'", val.type_name(), key);
            }
            std::println("Define variable {}={:.15g}", key, variables[key]);
        }
    }

    std::list<Reference> references;
    references.emplace_back("", nullptr, pos_t(0, 0, 0), Quaternion(0, 0, 0)); // dummy reference to global origin
    if (setup_desc.contains("references"))
    {
        for (auto& reference_desc : json_get(setup_desc, "references"))
        {
            factory::make_reference(reference_desc, references, variables);
            factory::assert_empty(reference_desc);
        }
    }

    std::list<std::unique_ptr<Radiator>> radiators;
    std::map<std::string, RadiatorArray> radiator_arrays;
    if (setup_desc.contains("radiators"))
    {
        for (auto& radiator_desc : json_get(setup_desc, "radiators"))
        {
            factory::make_radiator(radiator_desc, references, radiators, variables, radiator_arrays, false);
            factory::assert_empty(radiator_desc);
        }
    }

    std::list<std::pair<std::string, task_t>> tasks;
    if (setup_desc.contains("tasks"))
    {
        for (auto& task_desc : json_get(setup_desc, "tasks"))
        {
            auto const type = factory::get_string(task_desc, "type");
            std::println("Found task of type '{}'", type);
            std::string task_name;
            if (type == "builtin")
            {
                auto const key = factory::get_string(task_desc, "key");
                task_name = std::format("builtin.{}", key);
                tasks.emplace_back(task_name, nullptr);
            }
            else if (type == "plot_directivity_over_polar")
            {
                auto const azimuth_angles = factory::get_ndarray(task_desc, "azimuth_angles") * nc::constants::pi;
                Radiator const& radiator = factory::find_radiator_by_id(radiators, factory::get_string(task_desc, "radiator"));
                task_name = std::format("{}.{}", type, radiator.id);
                tasks.emplace_back(task_name, [&radiator, azimuth_angles](std::filesystem::path const& directory) { plot::plot_directivity_over_polar(directory, radiator, azimuth_angles); });
            }
            else if (type == "plot_gain_over_straight")
            {
                Radiator const& source = factory::find_radiator_by_id(radiators, factory::get_string(task_desc, "source"));
                Radiator const& sink = factory::find_radiator_by_id(radiators, factory::get_string(task_desc, "sink"));
                Reference& ref_start = factory::find_reference_by_id(references, factory::get_string(task_desc, "ref_start"));
                Reference const& ref_stop = factory::find_reference_by_id(references, factory::get_string(task_desc, "ref_stop"));
                double wavelength = factory::get_double(task_desc, "wavelength", variables);
                char distance_axis = factory::get_char(task_desc, "distance_axis");
                task_name = std::format("{}.{}.{}", type, source.id, sink.id);
                tasks.emplace_back(task_name, [&source, &sink, &ref_start, &ref_stop, wavelength, distance_axis](std::filesystem::path const& directory)
                                   { plot::plot_gain_over_straight(directory, source, sink, ref_start, ref_stop, wavelength, distance_axis); });
            }
            else if (type == "plot_gain_over_plane")
            {
                auto const source_id = factory::get_string(task_desc, "source");
                radiator_t source = radiator_arrays.contains(source_id) ? radiator_t{radiator_arrays.at(source_id)} : radiator_t{factory::find_radiator_by_id(radiators, source_id)};
                Radiator const& sink = factory::find_radiator_by_id(radiators, factory::get_string(task_desc, "sink"));
                Reference& ref_start = factory::find_reference_by_id(references, factory::get_string(task_desc, "ref_start"));
                Reference const& ref_axis1_max = factory::find_reference_by_id(references, factory::get_string(task_desc, "ref_axis1_max"));
                Reference const& ref_axis2_max = factory::find_reference_by_id(references, factory::get_string(task_desc, "ref_axis2_max"));
                double wavelength = factory::get_double(task_desc, "wavelength", variables);
                std::uint32_t n_points_axis1 = factory::get_uint(task_desc, "n_points_axis1", variables);
                std::uint32_t n_points_axis2 = factory::get_uint(task_desc, "n_points_axis2", variables);
                auto label_axis1 = factory::get_string(task_desc, "label_axis1");
                auto label_axis2 = factory::get_string(task_desc, "label_axis2");
                task_name = std::format("{}.{}.{}", type, source_id, sink.id);
                tasks.emplace_back(
                    task_name, [source, &sink, &ref_start, &ref_axis1_max, &ref_axis2_max, wavelength, n_points_axis1, n_points_axis2, label_axis1, label_axis2](std::filesystem::path const& directory)
                    { plot::plot_gain_over_plane(directory, source, sink, ref_start, ref_axis1_max, ref_axis2_max, wavelength, n_points_axis1, n_points_axis2, label_axis1, label_axis2); });
            }
            else
            {
                throw SimulationError("Unknown task type \"{}\"", type);
            }
            std::println("Created task: {}", task_name);
            factory::assert_empty(task_desc);
        }
    }

    // ReSharper disable once CppDFAMemoryLeak
    return std::unique_ptr<Setup>(new Setup(setup_name, timestamp, std::move(variables), std::move(references), std::move(radiators), std::move(radiator_arrays), std::move(tasks)));
}

std::unique_ptr<Setup> Setup::from_file(std::filesystem::path const& p)
{
    std::println("Loading setup file '{}'", p.string());
    std::ifstream file(p);
    if (!file.is_open()) { throw SimulationError("Could not open setup file '{}'", p.string()); }
    auto const js = nlohmann::ordered_json::parse(file);
    file.close();
    return from_json(js, timeutil::get_of_file(p));
}

void Setup::export_to_three(std::filesystem::path const& directory) const
{
    std::filesystem::path const p = directory / "objects.js";
    three::Container container;
    for (auto const& reference : references)
    {
        if (!reference.origin) { continue; } // we skip the dummy reference
        auto const pos_center = reference.global_from_local_pos({0, 0, 0});
        auto const pos_origin = reference.origin->global_from_local_pos({0, 0, 0});
        auto const distance = (pos_center - pos_origin).norm();
        auto const radius = distance * 1e-3;
        container.add(three::make_cylinder(pos_origin, pos_center, radius, radius, fmt::color::white));
        container.add(three::create_coordinate_arrows(pos_center, reference.global_from_local_pos({1, 0, 0}) - pos_center, reference.global_from_local_pos({0, 1, 0}) - pos_center,
                                                      reference.global_from_local_pos({0, 0, 1}) - pos_center, distance * 1e-1));
    }
    container.export_to_javascript(p);
}

void Setup::run_tasks(std::filesystem::path const& directory, std::function<void(std::string_view)> const& builtin_handler)
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
            task(directory);
        }
    }
    std::println("{}All tasks completed.{}", fg4::cyan, reset);
}

Reference& Setup::get_reference_by_id(std::string_view const id) { return factory::find_reference_by_id(references, id); }

Radiator const& Setup::get_radiator_by_id(std::string_view const id) const { return factory::find_radiator_by_id(radiators, id); }

// std::vector<std::reference_wrapper<Radiator>> Setup::get_radiator_array(std::string_view id) const
// {
//     std::vector<std::reference_wrapper<Radiator>> radiator_array;
//     std::string const id_prefix = std::format("{}:radiator:", id);
//     for (auto& radiator : radiators)
//     {
//         if (radiator->id.starts_with(id_prefix)) { radiator_array.push_back(std::ref(*radiator)); }
//     }
//     return radiator_array;
// }

complex_t Setup::calc_voltage_gain(Radiator const& radiator_tx, Radiator const& radiator_rx, double const wavelength, math::NumParams const& num_params)
{
    double const r = (radiator_tx.origin.global_from_local_pos(POS_ZERO) - radiator_rx.origin.global_from_local_pos(POS_ZERO)).norm();
    if (r < wavelength / 10) { std::println("Warning: Radiator {} is very close to radiator {}, distance: {} m ({} λ)", radiator_tx.id, radiator_rx.id, r, r / wavelength); }

    auto const pos_local_tx = radiator_tx.origin.localize(radiator_rx.origin); // position of rx radiator in tx coordinate
    auto const pos_local_rx = radiator_rx.origin.localize(radiator_tx.origin); // position of tx radiator in rx coordinate
    auto const rot_mat_tx = math::get_rot_mat_from_cartesian(pos_local_tx);
    auto const rot_mat_rx = math::get_rot_mat_from_cartesian(pos_local_rx);
    auto const elv_spherical_tx = radiator_tx.get_elv_spherical_from_cartesian(pos_local_tx, wavelength);
    auto const elv_spherical_rx = radiator_rx.get_elv_spherical_from_cartesian(pos_local_rx, wavelength);
    auto const elv_cartesian_tx = nc::dot(rot_mat_tx, elv_spherical_tx);
    auto const elv_cartesian_rx = nc::dot(rot_mat_rx, elv_spherical_rx);
    auto const elv_global_tx = radiator_tx.origin.global_from_local_vec(elv_cartesian_tx);
    auto const elv_global_rx = radiator_rx.origin.global_from_local_vec(elv_cartesian_rx);
    auto const g = elv_global_tx.dot(elv_global_rx).item();
    auto const propagation = std::exp(-j * 2.0 * pi * r / wavelength) * wavelength / (4.0 * pi * r);
    auto const mean_squared_elv_tx =
        radiator_tx.mean_squared_elv ? radiator_tx.mean_squared_elv(wavelength) : Radiator::calc_mean_squared_effective_length(radiator_tx.elv_spherical, wavelength, num_params);
    auto const mean_squared_elv_rx =
        radiator_rx.mean_squared_elv ? radiator_rx.mean_squared_elv(wavelength) : Radiator::calc_mean_squared_effective_length(radiator_rx.elv_spherical, wavelength, num_params);
    return -j * g / std::sqrt(mean_squared_elv_tx * mean_squared_elv_rx) * propagation;
}

complex_t Setup::calc_voltage_gain(RadiatorArray const& radiator_array_tx, Radiator const& radiator_rx, double wavelength, math::NumParams const& num_params)
{
    complex_t gain = 0;
    for (auto const& radiator_tx : radiator_array_tx.elements) { gain += calc_voltage_gain(radiator_tx, radiator_rx, wavelength, num_params); }
    return gain;
}

double Setup::calc_power_gain(Radiator const& radiator_tx, Radiator const& radiator_rx, double const wavelength, math::NumParams const& num_params)
{ return math::square(std::abs(calc_voltage_gain(radiator_tx, radiator_rx, wavelength, num_params))); }

double Setup::calc_power_gain(RadiatorArray const& radiator_array_tx, Radiator const& radiator_rx, double wavelength, math::NumParams const& num_params)
{ return math::square(std::abs(calc_voltage_gain(radiator_array_tx, radiator_rx, wavelength, num_params))); }

Setup::Setup(std::string_view const name, timeutil::timestamp_t const timestamp, std::map<std::string, double>&& variables, std::list<Reference>&& references,
             std::list<std::unique_ptr<Radiator>>&& radiators, std::map<std::string, RadiatorArray>&& radiator_arrays, std::list<std::pair<std::string, task_t>>&& tasks) :
    name(name), timestamp(timestamp), variables(std::move(variables)), references(std::move(references)), radiators(std::move(radiators)), radiator_arrays(std::move(radiator_arrays)),
    tasks(std::move(tasks))
{}
