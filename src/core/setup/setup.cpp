//
// Created by Tristan Krause on 2026-05-26.
//

#include "setup/setup.hpp"
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <magic_enum/magic_enum.hpp>
#include <print>
#include <variant>
#include "convert.hpp"
#include "eval/output.hpp"
#include "factory/find.hpp"
#include "factory/get.hpp"
#include "factory/make.hpp"
#include "factory/parse.hpp"
#include "logging.hpp"
#include "manifest.hpp"
#include "simulationerror.hpp"
#include "three.hpp"

namespace setup
{
    using reference::Reference;

    namespace
    {
        constexpr std::string_view LOG_NAME = "setup";

        ojson load_json(std::filesystem::path const& path_json)
        {
            std::ifstream file(path_json);
            if (!file.is_open()) { throw SimulationError("Could not open setup file '{}'", path_json.string()); }
            auto const js = nlohmann::ordered_json::parse(file);
            file.close();
            return js;
        }

        Setup::Meta extract_meta(ojson& js, timeutil::timestamp_t timestamp)
        {
            using namespace std::ranges;
            using namespace std::views;
            auto& metadata = js.at("metadata");

            auto name = factory::get_string(metadata, "setup_name");

            auto const version_str = factory::get_string(metadata, "version");
            std::array<int, 3> version;
            try
            {
                std::ranges::transform(version_str | split('.'), version.begin(), convert::int_from_string, [](auto&& r) { return std::string_view(r); });
            }
            catch (...)
            {
                std::throw_with_nested(SimulationError("Malformed version string: '{}'", version_str));
            }

            if (version[0] != APPLICATION_VERSION[0]) //
                throw SimulationError("Incompatible setup version: setup is v{}, but application expects major version {}.x.x",
                    convert::string_from_version(version),
                    APPLICATION_VERSION[0]);
            if (version[1] != APPLICATION_VERSION[1]) //
                antennavision::logging::warn( //
                    LOG_NAME,
                    "Warning: Potentially incompatible setup version: setup is v{}, but application expects version {}.{}.x",
                    convert::string_from_version(version),
                    APPLICATION_VERSION[0],
                    APPLICATION_VERSION[1]);

            factory::assert_empty(metadata);
            js.erase("metadata");
            return {
                .name = name,
                .version = version,
                .timestamp = timestamp //
            };
        }

        std::vector<Codebook> extract_codebooks(ojson& js, std::filesystem::path const& cwd)
        {
            std::vector<Codebook> codebooks;
            if (js.contains("codebooks"))
            {
                auto const& desc = js.at("codebooks");
                try
                {
                    for (auto& cb_desc : desc)
                    {
                        auto id = cb_desc.at("id").get<std::string>();
                        auto path = cwd / cb_desc.at("path").get<std::string>();
                        codebooks.push_back(std::move(Codebook::from_file(id, path)));
                    }
                    js.erase("codebooks");
                }
                catch (...)
                {
                    std::throw_with_nested(SimulationError("Failed to parse codebooks:\n{}'", desc.dump(2)));
                }
            }
            return std::move(codebooks);
        }

        VarMap extract_variables(ojson& js)
        {
            VarMap variables;
            variables["c0"] = c0; // speed of light in vacuum
            variables["epsilon0"] = epsilon0; // permittivity of vacuum (free space) / electric constant
            variables["mu0"] = mu0; // permeability of vacuum (free space) / magnetic constant
            variables["Z0"] = Z0; // impedance of free space

            if (!js.contains("variables")) { return std::move(variables); }
            for (auto& vars_desc = js.at("variables"); const auto& [raw_key, val] : vars_desc.items())
            {
                try
                {
                    auto const colon_pos = raw_key.find(':');
                    auto const stripped_key = std::string_view(raw_key).substr(0, colon_pos);
                    auto const type = colon_pos == std::string_view::npos ? std::string_view() : std::string_view(raw_key).substr(colon_pos + 1);
                    bool is_double = type.empty() or type == "double";
                    if (!is_double and type != "int") { throw SimulationError("Variable '{}' has invalid type specifier '{}'", stripped_key, type); }
                    std::string key(stripped_key);

                    if (variables.contains(key)) throw SimulationError("Variable '{}' already exists", key);

                    if (val.is_string())
                    {
                        if (is_double) { variables[key] = factory::parse_double(val.get<std::string>(), variables); }
                        else
                        {
                            variables[key] = factory::parse_int(val.get<std::string>(), variables);
                        }
                    }
                    else if (val.is_number())
                    {
                        if (is_double) { variables[key] = val.is_number_float() ? val.get<double>() : static_cast<double>(val.get<std::int64_t>()); }
                        else
                        {
                            variables[key] = val.is_number_float() ? static_cast<std::int64_t>(std::roundl(val.get<long double>())) : val.get<std::int64_t>();
                        }
                    }
                    else
                    {
                        throw SimulationError("Invalid type '{}' of variable '{}'", val.type_name(), stripped_key);
                    }
                }
                catch (...)
                {
                    std::throw_with_nested(SimulationError("Failed to process variable '{}'", raw_key));
                }
            }
            js.erase("variables");
            return std::move(variables);
        }

        SimParams extract_sim_params(ojson& js, VarMap const& variables)
        {
            SimParams sim_params = {};
            if (js.contains("sim_params"))
            {
                auto& desc = js.at("sim_params");
                try
                {
                    factory::try_resolve_double_expressions(desc, variables, "system_wavelength");
                    desc.get_to(sim_params);
                    js.erase("sim_params");
                }
                catch (...)
                {
                    std::throw_with_nested(SimulationError("Failed to parse numerical parameters:\n{}'", desc.dump(2)));
                }
            }
            return std::move(sim_params);
        }

        std::vector<Reference> extract_references(ojson& js, VarMap const& variables)
        {
            std::vector<Reference> references;
            references.emplace_back(); // dummy reference to global origin
            if (!js.contains("references")) { return std::move(references); }
            references.reserve(references.size() + js.at("references").size());
            for (auto& refs_desc = js.at("references"); auto& desc : refs_desc)
            { //
                references.push_back(factory::make_reference(desc, variables));
            }
            js.erase("references");
            return std::move(references);
        }

        std::vector<components::Antenna> extract_antennas(ojson& js, VarMap const& variables)
        {
            std::vector<components::Antenna> antennas;
            if (!js.contains("antennas")) { return std::move(antennas); }
            for (auto& ants_desc = js.at("antennas"); auto& desc : ants_desc)
            { //
                antennas.push_back(factory::make_antenna(desc, variables));
            }
            js.erase("antennas");
            return std::move(antennas);
        }

        std::vector<geometry::Geometry> extract_geometries(ojson& js, VarMap const& variables)
        {
            std::vector<geometry::Geometry> geometries;
            if (!js.contains("geometries")) { return std::move(geometries); }
            for (auto& geos_desc = js.at("geometries"); auto& desc : geos_desc)
            { //
                geometries.push_back(factory::make_geometry(desc, variables));
            }
            js.erase("geometries");
            return std::move(geometries);
        }

        std::vector<sweep::Sweep> extract_sweeps(ojson& js, VarMap const& variables)
        {
            std::vector<sweep::Sweep> sweeps;
            if (!js.contains("sweeps")) { return std::move(sweeps); }
            for (auto& sweeps_desc = js.at("sweeps"); auto& desc : sweeps_desc)
            { //
                sweeps.push_back(factory::make_sweep(desc, variables));
            }
            js.erase("sweeps");
            return std::move(sweeps);
        }

        std::vector<task::Task> extract_tasks(ojson& js, Context const& ctx)
        {
            std::vector<task::Task> tasks;
            if (!js.contains("tasks")) { return std::move(tasks); }
            for (auto& tasks_desc = js.at("tasks"); auto& desc : tasks_desc)
            {
                try
                {
                    auto t = task::from_json(desc, ctx);
                    (void)task::get_output_type(t); // assert that the task has a valid output type
                    tasks.push_back(t);
                } catch (...)
                {
                    std::throw_with_nested(SimulationError("Could not parse task:\n{}", desc.dump(2)));
                }
            }
            js.erase("tasks");
            return std::move(tasks);
        }

        std::vector<Complex> get_coeffs_from_codebook(std::span<Codebook const> codebooks, std::span<std::string const> key)
        {
            auto const& id_cb = key.at(0);
            auto it = std::ranges::find(codebooks, id_cb, &Codebook::id);
            if (it == codebooks.end()) throw SimulationError("Could not find codebook with id '{}'", id_cb);
            return (*it)[key.subspan(1)] | std::ranges::to<std::vector<Complex>>();
        }

        void print_reference(Reference const& ref, char const* indent)
        {
            auto const origin_id = ref.origin_id.empty() ? std::string("<global origin>") : std::format("'{}'", ref.origin_id);
            auto const pos = ref.global_from_local_pos(POS_ZERO);
            auto const ex = ref.global_from_local_pos(Pos(1, 0, 0)) - pos;
            auto const ey = ref.global_from_local_pos(Pos(0, 1, 0)) - pos;
            auto const ez = ref.global_from_local_pos(Pos(0, 0, 1)) - pos;
            antennavision::logging::info( //
                LOG_NAME,
                "{}Reference '{}' with origin ref {}: pos=({:.06f},{:.06f},{:.06f})"
                " ex=({:.03f},{:.03f},{:.03f}) ey=({:.03f},{:.03f},{:.03f}) ez=({:.03f},{:.03f},{:.03f})",
                indent,
                ref.id,
                origin_id,
                pos.x,
                pos.y,
                pos.z,
                ex.x,
                ex.y,
                ex.z,
                ey.x,
                ey.y,
                ey.z,
                ez.x,
                ez.y,
                ez.z //
            );
        }
    } // namespace

    Setup Setup::from_json(ojson const& desc, std::filesystem::path const& cwd, timeutil::timestamp_t timestamp)
    {
        ojson js = desc; // create a copy of the json object in order to decompose it
        Meta meta = extract_meta(js, timestamp);
        std::vector<Codebook> codebooks = extract_codebooks(js, cwd);
        VarMap variables = extract_variables(js);
        SimParams sim_params = extract_sim_params(js, variables);
        if (not variables.contains("system_wavelength")) variables["system_wavelength"] = sim_params.system_wavelength;
        std::vector<Reference> references = extract_references(js, variables);
        std::vector<components::Antenna> antennas = extract_antennas(js, variables);
        std::vector<geometry::Geometry> geometries = extract_geometries(js, variables);
        std::vector<sweep::Sweep> sweeps = extract_sweeps(js, variables);
        Context ctx{
            .codebooks = codebooks,
            .variables = variables,
            .references = references,
            .antennas = antennas,
            .geometries = geometries,
            .sweeps = sweeps //
        };
        std::vector<task::Task> tasks = extract_tasks(js, ctx);

        // check that the json contains no invalid (unknown) fields
        factory::assert_empty(js);

        // crucial: trace all origins by their id and connect the pointers
        reference::resolve_origins(references);
        components::antenna::rebind_origin_pointers(antennas, references);

        return {
            //
            .cwd = cwd, // copy here
            .meta = std::move(meta),
            .codebooks = std::move(codebooks),
            .sim_params = std::move(sim_params),
            .variables = std::move(variables),
            .references = std::move(references),
            .antennas = std::move(antennas),
            .geometries = std::move(geometries),
            .sweeps = std::move(sweeps),
            .tasks = std::move(tasks) //
        };
    }

    Setup Setup::from_file(std::filesystem::path const& path_json)
    { return std::move(from_json(load_json(path_json), path_json.parent_path(), timeutil::get_of_file(path_json))); }

    void Setup::reconcile()
    {
        // crucial: trace all origins by their id and connect the pointers
        reference::resolve_origins(references);
        components::antenna::rebind_origin_pointers(antennas, references);
    }

    void Setup::print_meta() const
    {
        antennavision::logging::info(LOG_NAME, "Setup name: {}", meta.name);
        antennavision::logging::info(LOG_NAME, "Setup version: {}", convert::string_from_version(meta.version));
    }

    void Setup::print_variables() const
    {
        using std::ranges::to;
        using std::views::elements;
        using std::views::transform;

        antennavision::logging::info(LOG_NAME, "Setup variables:");
        if (variables.empty())
        {
            antennavision::logging::info(LOG_NAME, "  No variables defined.");
            return;
        }

        auto const rendered_map = variables |
            transform(
                [](const auto& entry)
                {
                    Var const& var = std::get<1>(entry);
                    auto const is_double = std::holds_alternative<double>(var);
                    auto const val = is_double ? std::format("{:.17g}", std::get<double>(var)) : std::to_string(std::get<std::int64_t>(var));
                    return std::tuple{std::get<0>(entry), std::string(is_double ? "double" : "int"), std::move(val)};
                }) |
            to<std::vector<std::tuple<std::string, std::string, std::string>>>();

        std::size_t const max_len_name = std::ranges::max(rendered_map | elements<0>, {}, &std::string::length).length();
        std::size_t const max_len_type = std::ranges::max(rendered_map | elements<1>, {}, &std::string::length).length();
        std::size_t const max_len_value = std::ranges::max(rendered_map | elements<2>, {}, &std::string::length).length();

        antennavision::logging::info(LOG_NAME, "  X-{:-<{}}---{:-<{}}---{:-<{}}-X", "", max_len_name, "", max_len_type, "", max_len_value);
        for (auto const& [name, type, value] : rendered_map)
        {
            antennavision::logging::info(LOG_NAME, "  | {:<{}} | {:<{}} | {:<{}} |", name, max_len_name, type, max_len_type, value, max_len_value);
        }
        antennavision::logging::info(LOG_NAME, "  X-{:-<{}}---{:-<{}}---{:-<{}}-X", "", max_len_name, "", max_len_type, "", max_len_value);
    }

    void Setup::print_references() const
    {
        antennavision::logging::info(LOG_NAME, "Setup references:");
        if (antennas.empty())
        {
            antennavision::logging::info(LOG_NAME, "  No references defined.");
            return;
        }
        for (Reference const& ref : references)
        {
            if (not ref.id.empty()) print_reference(ref, "  * ");
        }
    }

    void Setup::print_antennas() const
    {
        antennavision::logging::info(LOG_NAME, "Setup antennas:");
        if (antennas.empty())
        {
            antennavision::logging::info(LOG_NAME, "  No antennas defined.");
            return;
        }

        for (auto const& ant : antennas)
        {
            ant.visit(
                [](const auto& a)
                {
                    using Type = std::decay_t<decltype(a)>;
                    if constexpr (std::is_same_v<Type, components::RadiatorArray>)
                    {
                        antennavision::logging::info(LOG_NAME, "  * Array '{}' of type {} placed at '{}'", a.id, magic_enum::enum_name(a.type), a.origin_id);
                        for (Reference const& ref : a.references) //
                            print_reference(ref, "    ~ ");
                        for (components::Radiator const& el : a.elements) //
                            std::println("    ~ Element '{}' of type {} placed at '{}'", el.id, magic_enum::enum_name(el.type), el.origin_id);
                    }
                    else if constexpr (std::is_same_v<components::Radiator, Type>)
                    {
                        antennavision::logging::info(LOG_NAME,
                            "  * Radiator '{}' of type {} with origin ref '{}'",
                            a.id,
                            magic_enum::enum_name(a.type),
                            a.origin_id);
                    }
                    else
                        std::unreachable();
                });
        }
    }

    void Setup::print_sim_params() const
    {
        antennavision::logging::info(LOG_NAME, "Simulation parameters:");
        antennavision::logging::info(LOG_NAME, "  * system wavelength: {:.17g}", sim_params.system_wavelength);
        antennavision::logging::info(LOG_NAME, "  * enable path loss:  {}", sim_params.enable_path_loss);
        antennavision::logging::info(LOG_NAME, "  * n polar:           {}", sim_params.n_polar);
        antennavision::logging::info(LOG_NAME, "  * n azimuth:         {}", sim_params.n_azimuth);
        antennavision::logging::info(LOG_NAME, "  * n linear1:         {}", sim_params.n_linear1);
        antennavision::logging::info(LOG_NAME, "  * n linear2:         {}", sim_params.n_linear2);
        antennavision::logging::info(LOG_NAME, "  * xtol rel:          {:.17g}", sim_params.xtol_rel);
        antennavision::logging::info(LOG_NAME, "  * ftol rel:          {:.17g}", sim_params.ftol_rel);
    }

    void Setup::export_to_three(std::filesystem::path const& path_objects) const
    {
        auto const& system_wavelength = sim_params.system_wavelength;

        three::Container container;
        for (auto const& reference : references)
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

        auto add_radiator = [&container, &system_wavelength](components::Radiator const& radiator)
        {
            auto pos_center = radiator.origin->global_from_local_pos(POS_ZERO);
            double const radiator_length = 0.49 * system_wavelength;
            auto pos_end = radiator.origin->global_from_local_pos({0.0, 0.0, 0.5 * radiator_length});
            auto const pos_start = pos_center - (pos_end - pos_center);
            double const radius = 0.1 * radiator_length;
            container.add(three::make_cylinder(pos_start, pos_end, radius, radius));
        };
        for (auto const& ant : antennas)
        {
            ant.visit(
                [&](auto const& ant)
                {
                    using Type = std::decay_t<decltype(ant)>;
                    if constexpr (std::is_same_v<Type, components::Radiator>) { add_radiator(ant); }
                    else if constexpr (std::is_same_v<Type, components::RadiatorArray>)
                    {
                        for (auto const& element : ant.elements) { add_radiator(element); }
                    }
                    else
                    {
                        throw SimulationError("Invalid antenna type");
                    }
                });
        }

        for (auto const& geo : geometries) container.add(three::export_geometry(geo));
        container.export_to_javascript(path_objects);
        antennavision::logging::info(LOG_NAME, "Exported objects to {}", path_objects.string());
    }

    void Setup::run_tasks(bool force_recomputation) const
    {
        if (meta.timestamp == 0) force_recomputation = true;

        std::filesystem::current_path(cwd);

        for (auto const& task : tasks)
        {
            auto const id = setup::task::get_id(task);

            auto const& output_path = task::get_output_path(task);
            bool const result_found = std::filesystem::is_regular_file(output_path) and std::filesystem::file_size(output_path) > 0;
            auto const timestamp_result = result_found ? timeutil::get_of_file(output_path) : 0;
            bool const up_to_date = timestamp_result > meta.timestamp;
            if (up_to_date and not force_recomputation)
            {
                antennavision::logging::info(LOG_NAME,
                    "Skipping computation: Output file (modified {}) is newer than setup file (modified {}).",
                    timeutil::format(timestamp_result),
                    timeutil::format(meta.timestamp));
                continue;
            }

            if (up_to_date)
                antennavision::logging::info(LOG_NAME, "Next task (forced):");
            else
                antennavision::logging::info(LOG_NAME, "Next task:");
            antennavision::logging::info(LOG_NAME, "  * name:         {}", setup::task::get_name(task));
            antennavision::logging::info(LOG_NAME, "  * id:           {}", id);
            antennavision::logging::info(LOG_NAME, "  * path output:  {}", output_path.string());

            task.visit(
                [&](auto const& t)
                {
                    using TaskType = std::decay_t<decltype(t)>;
                    if constexpr (std::is_same_v<TaskType, task::DirectivityOverPolarAtAzimuth>) { eval::output::directivity_over_polar(t, sim_params); }
                    else if constexpr (std::is_same_v<TaskType, task::VoltGainOverPoints>)
                    {
                        auto const tx_coeffs = t.tx_codebook.empty() //
                            ? std::vector<Complex>(components::antenna::size(t.tx), 1.0)
                            : get_coeffs_from_codebook(codebooks, t.tx_codebook);

                        auto const rx_coeffs = t.rx_codebook.empty() //
                            ? std::vector<Complex>(components::antenna::size(t.rx), 1.0)
                            : get_coeffs_from_codebook(codebooks, t.rx_codebook);

                        auto const field = eval::RxVoltageField(t.tx, t.rx, tx_coeffs, rx_coeffs, sim_params);
                        eval::output::voltgain::points(t, field);
                    }
                    else if constexpr (std::is_same_v<TaskType, task::VoltGainOverGeometry>)
                    {
                        auto const tx_coeffs = t.tx_codebook.empty() //
                            ? std::vector<Complex>(components::antenna::size(t.tx), 1.0)
                            : get_coeffs_from_codebook(codebooks, t.tx_codebook);

                        auto const rx_coeffs = t.rx_codebook.empty() //
                            ? std::vector<Complex>(components::antenna::size(t.rx), 1.0)
                            : get_coeffs_from_codebook(codebooks, t.rx_codebook);

                        auto const field = eval::RxVoltageField(t.tx, t.rx, tx_coeffs, rx_coeffs, sim_params);
                        eval::output::voltgain::geometry(t, field);
                    }
                    else if constexpr (std::is_same_v<TaskType, task::VoltGainOverGeometryAtWavelength>)
                    {
                        auto const tx_coeffs = t.tx_codebook.empty() //
                            ? std::vector<Complex>(components::antenna::size(t.tx), 1.0)
                            : get_coeffs_from_codebook(codebooks, t.tx_codebook);

                        auto const rx_coeffs = t.rx_codebook.empty() //
                            ? std::vector<Complex>(components::antenna::size(t.rx), 1.0)
                            : get_coeffs_from_codebook(codebooks, t.rx_codebook);

                        auto const field = eval::RxVoltageField(t.tx, t.rx, tx_coeffs, rx_coeffs, sim_params);
                        eval::output::voltgain::geometry_at_wavelength(t, field);
                    }
                    else if constexpr (std::is_same_v<TaskType, task::VoltGainPeakAndCutoffs>)
                    {
                        auto const tx_coeffs = t.tx_codebook.empty() //
                            ? std::vector<Complex>(components::antenna::size(t.tx), 1.0)
                            : get_coeffs_from_codebook(codebooks, t.tx_codebook);

                        auto const rx_coeffs = t.rx_codebook.empty() //
                            ? std::vector<Complex>(components::antenna::size(t.rx), 1.0)
                            : get_coeffs_from_codebook(codebooks, t.rx_codebook);

                        auto const field = eval::RxVoltageField(t.tx, t.rx, tx_coeffs, rx_coeffs, sim_params);
                        eval::output::voltgain::curve_peak_and_cutoff(t, field);
                    }
                });
        }
    }

    Reference const& Setup::get_reference(std::string_view id) { return reference::get(std::span<reference::Reference const>(references), id); }

    components::Antenna const& Setup::get_antenna(std::string_view id) { return components::antenna::get(std::span<components::Antenna const>(antennas), id); }

    Context Setup::get_context() const
    {
        return {
            .codebooks = codebooks,
            .variables = variables,
            .references = references,
            .antennas = antennas,
            .geometries = geometries,
            .sweeps = sweeps //
        };
    }

    double Setup::get_double(std::string const& variable_name) const
    {
        auto const var = variables.at(variable_name);
        auto const ptr = std::get_if<double>(&var);
        if (!ptr) { throw SimulationError("Variable '{}' is not a double", variable_name); }
        return *ptr;
    }

    std::int64_t Setup::get_int(std::string const& variable_name) const
    {
        auto const var = variables.at(variable_name);
        auto const ptr = std::get_if<std::int64_t>(&var);
        if (!ptr) { throw SimulationError("Variable '{}' is not an int", variable_name); }
        return *ptr;
    }
} // namespace setup
