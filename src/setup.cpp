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

    std::map<std::string, double> extract_variables(factory::Context const& context)
    {
        std::map<std::string, double> variables;
        if (context.setup_desc.contains("variables"))
        {
            for (const auto& [key, val] : context.setup_desc["variables"].items())
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
        return variables;
    }

    void extract_references(factory::Context& context)
    {
        std::list<Reference> references;
        references.emplace_back("", nullptr, pos_t(0, 0, 0), Quaternion(0, 0, 0)); // dummy reference to global origin
        if (context.setup_desc.contains("references"))
        {
            for (auto& reference_desc : json_get(context.setup_desc, "references"))
            {
                factory::make_reference(reference_desc, references, context.variables);
                factory::assert_empty(reference_desc);
            }
        }
    }

    void extract_antennas(factory::Context& context)
    {
        if (context.setup_desc.contains("radiators"))
        {
            for (auto& radiator_desc : json_get(context.setup_desc, "radiators"))
            {
                factory::make_antenna(radiator_desc, context, false);
                factory::assert_empty(radiator_desc);
            }
        }
    }

    void extract_tasks(factory::Context & context)
    {
        auto &setup_desc = context.setup_desc;
        auto &variables = context.variables;
        auto &references = context.references;
        auto &radiators = context.antennas;
        auto& tasks = context.tasks;
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
                    Antenna const& radiator = radiators.at(factory::get_string(task_desc, "radiator"));
                    task_name = std::format("{}.{}", type, antenna::get_id(radiator));
                    tasks.emplace_back(task_name, [&radiator, azimuth_angles](std::filesystem::path const& directory)
                                       { plot::plot_directivity_over_polar(directory, radiator, azimuth_angles); });
                }
                else if (type == "plot_gain_over_straight")
                {
                    Antenna const& source = radiators.at(factory::get_string(task_desc, "source"));
                    Antenna const& sink = radiators.at(factory::get_string(task_desc, "sink"));
                    Reference& ref_start = factory::find_reference_by_id(references, factory::get_string(task_desc, "ref_start"));
                    Reference const& ref_stop = factory::find_reference_by_id(references, factory::get_string(task_desc, "ref_stop"));
                    double wavelength = factory::get_double(task_desc, "wavelength", variables);
                    char distance_axis = factory::get_char(task_desc, "distance_axis");
                    task_name = std::format("{}.{}.{}", type, antenna::get_id(source), antenna::get_id(sink));
                    tasks.emplace_back(task_name, [&source, &sink, &ref_start, &ref_stop, wavelength, distance_axis](std::filesystem::path const& directory)
                                       { plot::plot_gain_over_straight(directory, source, sink, ref_start, ref_stop, wavelength, distance_axis); });
                }
                else if (type == "plot_gain_over_plane")
                {
                    auto const source_id = factory::get_string(task_desc, "source");
                    Antenna source = factory::find_radiator_by_id(radiators, source_id);
                    Antenna const& sink = context.antennas.at(factory::get_string(task_desc, "sink"));
                    Reference& ref_start = factory::find_reference_by_id(references, factory::get_string(task_desc, "ref_start"));
                    Reference const& ref_axis1_max = factory::find_reference_by_id(references, factory::get_string(task_desc, "ref_axis1_max"));
                    Reference const& ref_axis2_max = factory::find_reference_by_id(references, factory::get_string(task_desc, "ref_axis2_max"));
                    double wavelength = factory::get_double(task_desc, "wavelength", variables);
                    std::uint32_t n_points_axis1 = factory::get_uint(task_desc, "n_points_axis1", variables);
                    std::uint32_t n_points_axis2 = factory::get_uint(task_desc, "n_points_axis2", variables);
                    auto label_axis1 = factory::get_string(task_desc, "label_axis1");
                    auto label_axis2 = factory::get_string(task_desc, "label_axis2");
                    task_name = std::format("{}.{}.{}", type, source_id, antenna::get_id(sink));
                    tasks.emplace_back(task_name,
                                       [&source, &sink, &ref_start, &ref_axis1_max, &ref_axis2_max, wavelength, n_points_axis1, n_points_axis2, label_axis1,
                                        label_axis2](std::filesystem::path const& directory)
                                       {
                                           plot::plot_gain_over_plane(directory, source, sink, ref_start, ref_axis1_max, ref_axis2_max, wavelength,
                                                                      n_points_axis1, n_points_axis2, label_axis1, label_axis2);
                                       });
                }
                else
                {
                    throw SimulationError("Unknown task type \"{}\"", type);
                }
                std::println("Created task: {}", task_name);
                factory::assert_empty(task_desc);
            }
        }
    }
} // namespace

std::unique_ptr<Setup> Setup::from_json(nlohmann::ordered_json const& js, timeutil::timestamp_t const timestamp)
{
    ojson setup_desc = js; // create a copy of the json object in order to decompose it
    auto& metadata = json_get(setup_desc, "metadata");
    std::string_view const setup_name = json_get(metadata, "setup_name").get<std::string_view>();
    std::println("Setup name: {}", setup_name);

    factory::Context context{.setup_desc = setup_desc};
    extract_variables(context);
    extract_references(context);
    extract_antennas(context);
    extract_tasks(context);

    // ReSharper disable once CppDFAMemoryLeak
    return std::unique_ptr<Setup>(new Setup(setup_name, timestamp, std::move(context)));
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
    for (auto const& radiator : radiators)
    {
        auto pos_center = radiator->origin.global_from_local_pos(POS_ZERO);
        double const radiator_length = 0.49 * system_wavelength;
        auto pos_end = radiator->origin.global_from_local_pos({0.0, 0.0, 0.5 * radiator_length});
        auto const pos_start = pos_center - (pos_end - pos_center);
        double const radius = 0.1 * radiator_length;
        container.add(three::make_cylinder(pos_start, pos_end, radius, radius));
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

Reference& Setup::get_reference_by_id(std::string_view const id) { return factory::find_reference_by_id(references, id); }

Radiator& Setup::get_radiator_by_id(std::string_view const id) const { return factory::find_radiator_by_id(radiators, id); }

ScalarField Setup::get_voltage_field(Antenna const& radiator_array_tx, Antenna& radiator_rx, math::NumParams const& num_params)
{
    auto tx = std::get_if<UniformLinearArray>(&radiator_array_tx);
    assert(tx);
    auto rx = std::get_if<Radiator>(&radiator_array_tx);
    assert(rx);
    return {[&tx=*tx, &rx=*rx, num_params](pos_t const& pos, double const wavelength) -> complex_t
            {
                rx.origin.pos = pos;
                return calc_voltage_gain(tx, rx, wavelength, num_params);
            },
            [&rx=*rx] { rx.origin.reset(); }, num_params};
}

complex_t Setup::calc_voltage_gain(Radiator const& GenericRadiatorx, Radiator const& radiator_rx, double const wavelength, math::NumParams const& num_params)
{
    double const r = (GenericRadiatorx.origin.global_from_local_pos(POS_ZERO) - radiator_rx.origin.global_from_local_pos(POS_ZERO)).norm();
    if (r < wavelength / 10)
    {
        std::println("Warning: Radiator {} is very close to radiator {}, distance: {} m ({} λ)", GenericRadiatorx.id, radiator_rx.id, r, r / wavelength);
    }

    auto const pos_local_tx = GenericRadiatorx.origin.localize(radiator_rx.origin); // position of rx radiator in tx coordinate
    auto const pos_local_rx = radiator_rx.origin.localize(GenericRadiatorx.origin); // position of tx radiator in rx coordinate
    auto const rot_mat_tx = math::get_rot_mat_from_cartesian(pos_local_tx);
    auto const rot_mat_rx = math::get_rot_mat_from_cartesian(pos_local_rx);
    auto const elv_spherical_tx = GenericRadiatorx.get_elv_spherical_from_cartesian(pos_local_tx, wavelength);
    auto const elv_spherical_rx = radiator_rx.get_elv_spherical_from_cartesian(pos_local_rx, wavelength);
    auto const elv_cartesian_tx = nc::dot(rot_mat_tx, elv_spherical_tx);
    auto const elv_cartesian_rx = nc::dot(rot_mat_rx, elv_spherical_rx);
    auto const elv_global_tx = GenericRadiatorx.origin.global_from_local_vec(elv_cartesian_tx);
    auto const elv_global_rx = radiator_rx.origin.global_from_local_vec(elv_cartesian_rx);
    auto const g = elv_global_tx.dot(elv_global_rx).item();
    auto const propagation = std::exp(-j * 2.0 * pi * r / wavelength) * wavelength / (4.0 * pi * r);
    auto const mean_squared_elv_tx = GenericRadiatorx.mean_squared_elv
        ? GenericRadiatorx.mean_squared_elv(wavelength)
        : Radiator::calc_mean_squared_effective_length(GenericRadiatorx.elv_spherical, wavelength, num_params);
    auto const mean_squared_elv_rx = radiator_rx.mean_squared_elv
        ? radiator_rx.mean_squared_elv(wavelength)
        : Radiator::calc_mean_squared_effective_length(radiator_rx.elv_spherical, wavelength, num_params);
    return -j * g / std::sqrt(mean_squared_elv_tx * mean_squared_elv_rx) * propagation;
}

complex_t Setup::calc_voltage_gain(RadiatorArray const& radiator_array_tx, Radiator const& radiator_rx, double wavelength, math::NumParams const& num_params)
{
    complex_t gain = 0;
    for (auto const& GenericRadiatorx : radiator_array_tx.elements) { gain += calc_voltage_gain(GenericRadiatorx, radiator_rx, wavelength, num_params); }
    return gain;
}

double Setup::calc_power_gain(Radiator const& GenericRadiatorx, Radiator const& radiator_rx, double const wavelength, math::NumParams const& num_params)
{ return math::square(std::abs(calc_voltage_gain(GenericRadiatorx, radiator_rx, wavelength, num_params))); }

double Setup::calc_power_gain(RadiatorArray const& radiator_array_tx, Radiator const& radiator_rx, double wavelength, math::NumParams const& num_params)
{ return math::square(std::abs(calc_voltage_gain(radiator_array_tx, radiator_rx, wavelength, num_params))); }

bool Setup::isUpToDate(std::filesystem::path const& path_timestamp) const
{
    timeutil::timestamp_t const saved_timestamp = std::filesystem::exists(path_timestamp) ? timeutil::load_from_file(path_timestamp) : 0;

    // we skip if the timestamps match and are non-zero
    // zero timestamps are used by the testing framework to force setup's tasks execution
    return saved_timestamp && saved_timestamp == timestamp;
}

Setup::VoltageField::VoltageField(RadiatorArray const& radiator_array_tx, Radiator& radiator_rx) :
    radiator_array_tx(radiator_array_tx), radiator_rx(radiator_rx)
{}

complex_t Setup::VoltageField::calc_voltage_gain(pos_t pos, double wavelength) {}

Setup::Setup(std::string_view const name, timeutil::timestamp_t const timestamp, Context&& context) :
    name(name), timestamp(timestamp), variables(std::move(context.variables)), references(std::move(context.references)),
    radiators(std::move(context.radiators)), radiator_arrays(std::move(context.radiator_arrays)), tasks(std::move(context.tasks))
{}
