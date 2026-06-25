//
// Created by Tristan Krause on 2026-05-26.
//

#include "setup.hpp"
#include <algorithm>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include "factory/find.hpp"
#include "factory/get.hpp"
#include "factory/make.hpp"
#include "factory/parse.hpp"
#include "plot.hpp"
#include "print.hpp"
#include "three.hpp"

namespace
{
    template <typename ContainerType>
    ContainerType &json_get(ContainerType &js, std::string_view key)
    {
        factory::assert_key(js, key);
        return js[key];
    }

    // template <typename T>
    //     auto json_get_as(json const &js, std::string_view const key)
    // {
    //     if (!js.contains(key)) { throw std::runtime_error(std::format("Could not find key '{}' in json object\n{}", key, js.dump(2))); }
    //     if constexpr (std::is_same_v<T, NdArray>)
    //     {
    //         auto const phis = js[key].get<std::vector<double>>();
    //         return NdArray(phis.begin(), phis.end());
    //     }
    //     else if constexpr (std::is_same_v<T, char>)
    //     {
    //         std::string_view const str = js[key].get<std::string_view>();
    //         if (str.length() != 1) { throw std::runtime_error(std::format("Invalid character entry '{}', expected string of unity length.", str)); }
    //         return str.at(0);
    //     }
    //     else
    //     {
    //         return js[key].get<T>();
    //     }
    // }

    // template <typename T>
    // std::optional<T> optional_from_json(json const &j)
    // { return j.is_null() ? std::nullopt : std::optional<T>{j.get<T>()}; }

} // namespace

std::unique_ptr<Setup> Setup::from_json(nlohmann::ordered_json const &js)
{
    json setup_desc = js; // create a copy of the json object in order to decompose it
    auto const &metadata = json_get(setup_desc, "metadata");
    std::string_view const setup_name = json_get(metadata, "setup_name").get<std::string_view>();
    std::println("Setup name: {}", setup_name);

    std::map<std::string, double> variables;
    if (setup_desc.contains("variables"))
    {
        for (const auto &[key, val] : setup_desc["variables"].items())
        {
            if (val.is_string()) { variables[key] = factory::parse_double(val.get<std::string>(), variables); }
            else if (val.is_number()) { variables[key] = val.get<double>(); }
            else
            {
                throw std::runtime_error(std::format("Invalid type '{}' of variable '{}'", val.type_name(), key));
            }
            std::println("Define variable {}={:.15g}", key, variables[key]);
        }
    }

    std::list<Reference> references;
    references.emplace_back("", nullptr, Vec3(0, 0, 0), Quaternion(0, 0, 0)); // dummy reference to global origin
    if (setup_desc.contains("references"))
    {
        for (auto &reference_desc : json_get(setup_desc, "references"))
        {
            factory::make_reference(reference_desc, references, variables);
            factory::assert_empty(reference_desc);
        }
    }

    std::list<std::unique_ptr<Radiator>> radiators;
    if (setup_desc.contains("radiators"))
    {
        for (auto &radiator_desc : json_get(setup_desc, "radiators"))
        {
            factory::make_radiator(radiator_desc, references, radiators, variables, false);
            factory::assert_empty(radiator_desc);
        }
    }

    std::list<std::pair<std::string, std::function<void()>>> tasks;
    if (setup_desc.contains("tasks"))
    {
        auto const dir_plot = std::filesystem::path(setup_name) / "plots";
        for (auto &task_desc : json_get(setup_desc, "tasks"))
        {
            auto const type = factory::get_string(task_desc, "type");
            std::println("Found task of type '{}'", type);
            std::string task_name;
            if (type == "builtin")
            {
                auto const key = factory::get_string(task_desc, "key");
                task_name = std::format("builtin.{}", key);
                tasks.emplace_back(task_name, [] {});
            }
            else if (type == "plot_directivity_over_theta")
            {
                auto const phis = factory::get_ndarray(task_desc, "phis") * nc::constants::pi;
                Radiator const &radiator = factory::find_radiator_by_id(radiators, factory::get_string(task_desc, "radiator"));
                task_name = std::format("{}.{}", type, radiator.id);
                tasks.emplace_back(task_name, [dir_plot, &radiator, phis]() { plot::plot_directivity_over_theta(dir_plot, radiator, phis); });
            }
            else if (type == "plot_gain_over_straight")
            {
                Radiator const &source = factory::find_radiator_by_id(radiators, factory::get_string(task_desc, "source"));
                Radiator const &sink = factory::find_radiator_by_id(radiators, factory::get_string(task_desc, "sink"));
                Reference &ref_start = factory::find_reference_by_id(references, factory::get_string(task_desc, "ref_start"));
                Reference const &ref_stop = factory::find_reference_by_id(references, factory::get_string(task_desc, "ref_stop"));
                double wavelength = factory::get_double(task_desc, "wavelength", variables);
                char distance_axis = factory::get_char(task_desc, "distance_axis");
                task_name = std::format("{}.{}.{}", type, source.id, sink.id);
                tasks.emplace_back(task_name, [dir_plot, &source, &sink, &ref_start, &ref_stop, wavelength, distance_axis]()
                                   { plot::plot_gain_over_straight(dir_plot, source, sink, ref_start, ref_stop, wavelength, distance_axis); });
            }
            else
            {
                throw std::runtime_error(std::format("Unknown task type \"{}\"", type));
            }
            std::println("Created task: {}", task_name);
            factory::assert_empty(task_desc);
        }
    }

    // ReSharper disable once CppDFAMemoryLeak
    return std::unique_ptr<Setup>(new Setup(setup_name, std::move(variables), std::move(references), std::move(radiators), std::move(tasks)));
}

std::unique_ptr<Setup> Setup::from_file(std::filesystem::path const &p)
{
    std::println("Loading setup file '{}'", p.string());
    std::ifstream file(p);
    if (!file.is_open()) { throw std::runtime_error(std::format("Could not open setup file '{}'", p.string())); }
    auto const js = nlohmann::ordered_json::parse(file);
    file.close();
    return from_json(js);
}

void Setup::export_to_three() const
{
    std::filesystem::path const dir(name);
    std::filesystem::create_directories(dir);
    std::filesystem::path const p = dir / "objects.js";
    three::Container container;
    for (auto const &reference : references)
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

void Setup::run_tasks(const std::function<void(std::string_view)> &builtin_handler)
{
    for (auto &[task_name, task] : tasks)
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
    std::println("All tasks completed.");
}

Reference &Setup::get_reference_by_id(std::string_view const id) { return factory::find_reference_by_id(references, id); }

Radiator const &Setup::get_radiator_by_id(std::string_view const id) const { return factory::find_radiator_by_id(radiators, id); }

std::vector<Radiator *> Setup::get_radiator_array(std::string_view id) const
{
    std::vector<Radiator *> radiator_array;
    std::string const id_prefix = std::format("{}:radiator:", id);
    for (auto &radiator : radiators)
    {
        if (radiator->id.starts_with(id_prefix)) { radiator_array.push_back(radiator.get()); }
    }
    return radiator_array;
}

Setup::Setup(std::string_view const name, std::map<std::string, double> &&variables, std::list<Reference> &&references, std::list<std::unique_ptr<Radiator>> &&radiators,
             std::list<std::pair<std::string, std::function<void()>>> &&tasks) :
    name(name), variables(std::move(variables)), references(std::move(references)), radiators(std::move(radiators)), tasks(std::move(tasks))
{}
