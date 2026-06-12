//
// Created by Tristan Krause on 2026-05-26.
//

#include "setup.hpp"

#include <algorithm>
#include <exprtk.hpp>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include "components/customradiator.hpp"
#include "components/hertziandipole.hpp"
#include "components/isotropicradiator.hpp"
#include "manifest.hpp"
#include "plot.hpp"
#include "print.hpp"
#include "three.hpp"

namespace
{
    template <typename T>
    json const &json_get(json const &js, T *key)
    {
        if (js.contains(key)) { return js[key]; }
        throw std::runtime_error(std::format("Error: Could not find key '{}' in json object\n{}", key, js.dump(2)));
    }

    template <typename T>
    auto json_get_as(json const &js, std::string_view const key)
    {
        if constexpr (std::is_same_v<T, NdArray>)
        {
            if (js.contains(key))
            {
                auto const phis = js[key].get<std::vector<double>>();
                return NdArray(phis.begin(), phis.end());
            }
        }
        else
        {
            if (js.contains(key)) { return js[key].get<T>(); }
        }
        throw std::runtime_error(std::format("Error: Could not find key '{}' in json object\n{}", key, js.dump(2)));
    }

    Vec3 constexpr vec3_from_json(json const &js)
    {
        if (js.is_array()) { return {js.at(0).get<double>(), js.at(1).get<double>(), js.at(2).get<double>()}; }
        else { return {js.at("x").get<double>(), js.at("y").get<double>(), js.at("z").get<double>()}; }
    }

    Quaternion constexpr quaternion_from_json(json const &js)
    {
        if (js.is_array()) { return {js.at(2).get<double>() * PI, js.at(1).get<double>() * PI, js.at(0).get<double>() * PI}; }
        return {json_get(js, "roll").get<double>() * PI, json_get(js, "pitch").get<double>() * PI, json_get(js, "yaw").get<double>() * PI};
    }

    template <typename T>
    std::optional<T> optional_from_json(json const &j)
    {
        return j.is_null() ? std::nullopt : std::optional<T>{j.get<T>()};
    }

    Reference const &find_reference_by_id(decltype(Setup::references) const &references, std::string_view const id)
    {
        auto const it = std::ranges::find_if(references, [id](Reference const &reference) { return reference.id == id; });
        if (it != references.end()) { return *it; }
        throw std::runtime_error(std::format("Error: Could not find reference with id '{}'", id));
    }

    Radiator const &find_radiator_by_id(decltype(Setup::radiators) const &radiators, std::string_view const id)
    {
        auto const it = std::ranges::find_if(radiators, [id](std::unique_ptr<Radiator> const &radiator_ptr) { return radiator_ptr->id == id; });
        if (it != radiators.end()) { return **it; }
        throw std::runtime_error(std::format("Error: Could not find radiator with id '{}'", id));
    }

    std::function<double(double, double)> parse_theta_phi_function(std::string_view const expression)
    {
        // Struct to hold all ExprTk internal state variables safely on the heap
        struct ExpressionContext
        {
            double theta = 0.0;
            double phi = 0.0;
            exprtk::symbol_table<double> symbol_table;
            exprtk::expression<double> expression;
        };

        // Allocate the context
        auto ctx = std::make_shared<ExpressionContext>();

        // Bind variables to the symbol table
        ctx->symbol_table.add_variable("theta", ctx->theta);
        ctx->symbol_table.add_variable("phi", ctx->phi);
        ctx->symbol_table.add_constants();

        ctx->expression.register_symbol_table(ctx->symbol_table);

        // Compile the string
        exprtk::parser<double> parser;
        if (!parser.compile(std::string(expression), ctx->expression)) { throw std::runtime_error("ExprTk compilation failed: " + parser.error()); }

        // Return the callable lambda.
        // Capturing 'ctx' by value extends the lifetime of the underlying ExprTk objects.
        return [ctx](double const theta, double const phi) -> double
        {
            ctx->theta = theta;
            ctx->phi = phi;
            return ctx->expression.value();
        };
    }
} // namespace

Setup Setup::from_json(json const &js)
{
    auto const &metadata = json_get(js, "metadata");
    std::string_view const setup_name = json_get(metadata, "setup_name").get<std::string_view>();
    std::println("loading setup '{}'", setup_name);

    std::deque<Reference> references;
    Reference const &global_origin = references.emplace_back("", nullptr, Vec3(0, 0, 0), Quaternion(0, 0, 0)); // dummy reference to global origin
    if (js.contains("references"))
    {
        for (auto const &reference : json_get(js, "references"))
        {
            auto const id = json_get_as<std::string_view>(reference, "id");
            auto const origin_id = optional_from_json<std::string_view>(json_get(reference, "origin"));
            auto const translation = vec3_from_json(json_get(reference, "translation"));
            auto const rotation = quaternion_from_json(json_get(reference, "rotation"));
            std::println("loading reference [id: '{}', origin: '{}', translation: (x={:.3f}, y={:.3f}, z={:.3f}), rotation: (yaw={:.3f}π, pitch={:.3f}π, roll={:.3f}π]", id,
                         origin_id.value_or("<null>"), translation.x, translation.y, translation.z, rotation.yaw() / PI, rotation.pitch() / PI, rotation.roll() / PI);
            Reference const &origin = origin_id ? find_reference_by_id(references, origin_id.value()) : global_origin;
            references.emplace_back(id, &origin, translation, rotation);
        }
    }

    std::deque<std::unique_ptr<Radiator>> radiators;
    if (js.contains("radiators"))
    {
        for (auto const &radiator : json_get(js, "radiators"))
        {
            auto const id = json_get_as<std::string_view>(radiator, "id");
            auto const origin_id = json_get_as<std::string_view>(radiator, "reference");
            auto const type = json_get_as<std::string_view>(radiator, "type");
            std::println("loading radiator [id: '{}', origin: '{}', type: '{}']", id, origin_id, type);
            Reference const &origin = find_reference_by_id(references, origin_id);
            if (type == "HertzianDipole") { radiators.push_back(std::make_unique<HertzianDipole>(id, origin, json_get_as<double>(radiator, "length"))); }
            else if (type == "CustomRadiator")
            {
                auto const effective_length_defs = json_get_as<std::array<std::string_view, 3>>(radiator, "effective_length");
                std::array<std::function<double(double, double)>, 3> effective_length_parts;
                std::ranges::transform(effective_length_defs, effective_length_parts.begin(), parse_theta_phi_function);
                auto effective_length = [effective_length_parts](double const theta, double const phi) -> Vec3 {
                    return {effective_length_parts[0](theta, phi), effective_length_parts[1](theta, phi), effective_length_parts[2](theta, phi)};
                };
                radiators.push_back(std::make_unique<CustomRadiator>(id, origin, std::move(effective_length)));
            }
            else { throw std::runtime_error(std::format("unknown radiator type '{}'", type)); }
        }
    }

    std::deque<std::pair<std::string, std::function<void()>>> tasks;
    if (js.contains("tasks"))
    {
        path const dir_plot = path(setup_name) / "plots";
        for (auto const &task : json_get(js, "tasks"))
        {
            auto const type = json_get_as<std::string_view>(task, "type");
            std::println("loading task {}", type);
            if (type == "plot_directivity_over_theta")
            {
                auto const radiator_id = json_get_as<std::string_view>(task, "radiator");
                auto const phis = json_get_as<NdArray>(task, "phis") * PI;
                Radiator const &radiator = find_radiator_by_id(radiators, radiator_id);
                std::string task_name = std::format("{}.{}", type, radiator.id);
                std::println("creating task: {}", task_name);
                tasks.emplace_back(task_name, [dir_plot, &radiator, phis]() { plot::plot_directivity_over_theta(dir_plot, radiator, phis); });
            }
            else { throw std::runtime_error(std::format("unknown task type \"{}\"", type)); }
        }
    }

    return {setup_name, std::move(references), std::move(radiators), std::move(tasks)};
}

Setup Setup::from_file(path const &p)
{
    std::ifstream file(p);
    if (!file.is_open()) { throw std::runtime_error(std::format("Failed to open setup file: {}", p.string())); }
    nlohmann::json const js = nlohmann::json::parse(file);
    file.close();

    return from_json(js);
}

void Setup::export_to_three() const
{
    path const dir(name);
    std::filesystem::create_directories(dir);
    path const p = dir / "objects.js";
    three::Container container;
    for (auto const &reference : references)
    {
        if (!reference.origin) { continue; } // we skip the dummy reference
        auto const pos_center = reference.global_from_local({0, 0, 0});
        auto const pos_origin = reference.origin->global_from_local({0, 0, 0});
        auto const distance = (pos_center - pos_origin).norm();
        auto const radius = distance * 1e-3;
        container.add(three::make_cylinder(pos_origin, pos_center, radius, radius, fmt::color::white));
        container.add(three::create_coordinate_arrows(pos_center, reference.global_from_local({1, 0, 0}) - pos_center, reference.global_from_local({0, 1, 0}) - pos_center,
                                                      reference.global_from_local({0, 0, 1}) - pos_center, distance * 1e-1));
    }
    container.export_to_javascript(p);
}

void Setup::run_tasks()
{
    for (auto &[task_name, task] : tasks)
    {
        std::println("running task: {}", task_name);
        task();
    }
    std::println("all tasks completed.");
}

Reference const &Setup::get_reference_by_id(std::string_view const id) const { return find_reference_by_id(references, id); }

Radiator const &Setup::get_radiator_by_id(std::string_view const id) const { return find_radiator_by_id(radiators, id); }

Setup::Setup(std::string_view const name, std::deque<Reference> &&references, std::deque<std::unique_ptr<Radiator>> &&radiators, std::deque<std::pair<std::string, std::function<void()>>> &&tasks) :
    name(name), references(std::move(references)), radiators(std::move(radiators)), tasks(std::move(tasks))
{}
