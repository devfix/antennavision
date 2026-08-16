//
// Created by Tristan Krause on 2026-05-26.
//

#include "setup/setup.hpp"
#include <ansi_color.hpp>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <print>
#include <variant>
#include "convert.hpp"
#include "eval/output.hpp"
#include "factory/find.hpp"
#include "factory/get.hpp"
#include "factory/make.hpp"
#include "factory/parse.hpp"
#include "lg.hpp"
#include "manifest.hpp"
#include "simulationerror.hpp"
#include "three.hpp"

namespace setup
{
    using reference::Reference;

    namespace
    {
        ojson load_json(std::filesystem::path const& path_json)
        {
            std::ifstream file(path_json);
            if (!file.is_open()) { throw SimulationError("Could not open setup file '{}'", path_json.string()); }
            auto const js = nlohmann::ordered_json::parse(file);
            file.close();
            return js;
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
            lg::println( //
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

    Setup::Setup(std::filesystem::path const& path_json) : Setup(load_json(path_json), path_json.parent_path())
    { timestamp_ = timeutil::get_of_file(path_json); }

    Setup::Setup(ojson const& js_in, std::filesystem::path path_cwd) : path_cwd_(std::move(path_cwd))
    {
        ojson js = js_in; // create a copy of the json object in order to decompose it
        extract_meta(js);
        extract_codebooks(js);
        extract_variables(js);
        extract_sim_params(js);
        extract_references(js);
        extract_antennas(js);
        extract_geometries(js);
        extract_sweeps(js);
        extract_tasks(js);

        // check that the json contains no invalid (unknown) fields
        factory::assert_empty(js);

        reconcile();
    }

    void Setup::reconcile()
    {
        // crucial: trace all origins by their id and connect the pointers
        reference::resolve_origins(references_);
        antenna::rebind_origin_pointers(antennas_, references_);
    }

    void Setup::print_meta() const
    {
        lg::println("Setup name: {}", name_);
        lg::println("Setup version: {}", convert::string_from_version(version_));
    }

    void Setup::print_variables() const
    {
        using std::ranges::to;
        using std::views::elements;
        using std::views::transform;

        lg::println("Setup variables:");
        if (variables_.empty())
        {
            lg::println("  No variables defined.");
            return;
        }

        auto const rendered_map = variables_ |
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

        lg::println("  X-{:-<{}}---{:-<{}}---{:-<{}}-X", "", max_len_name, "", max_len_type, "", max_len_value);
        for (auto const& [name, type, value] : rendered_map)
        {
            lg::println("  | {:<{}} | {:<{}} | {:<{}} |", name, max_len_name, type, max_len_type, value, max_len_value);
        }
        lg::println("  X-{:-<{}}---{:-<{}}---{:-<{}}-X", "", max_len_name, "", max_len_type, "", max_len_value);
    }

    void Setup::print_references() const
    {
        lg::println("Setup references:");
        if (antennas_.empty())
        {
            lg::println("  No references defined.");
            return;
        }
        for (Reference const& ref : references_)
        {
            if (not ref.id.empty()) print_reference(ref, "  * ");
        }
    }

    void Setup::print_antennas() const
    {
        lg::println("Setup antennas:");
        if (antennas_.empty())
        {
            lg::println("  No antennas defined.");
            return;
        }

        for (auto const& ant : antennas_)
        {
            ant.visit(
                [](const auto& a)
                {
                    using Type = std::decay_t<decltype(a)>;
                    if constexpr (std::is_base_of_v<RadiatorArray<Type>, Type>)
                    {
                        lg::println("  * Array '{}' of type {} placed at '{}'", a.id, magic_enum::enum_name(a.type), a.origin_id);
                        for (Reference const& ref : a.references) //
                            print_reference(ref, "    ~ ");
                        for (Radiator const& el : a.elements) //
                            std::println("    ~ Element '{}' of type {} placed at '{}'", el.id, magic_enum::enum_name(el.type), el.origin_id);
                    }
                    else if constexpr (std::is_same_v<Radiator, Type>)
                    {
                        lg::println("  * Radiator '{}' of type {} with origin ref '{}'", a.id, magic_enum::enum_name(a.type), a.origin_id);
                    }
                    else
                        std::unreachable();
                });
        }
    }

    void Setup::print_sim_params() const
    {
        lg::println(lg::note, "Simulation parameters:");
        lg::println(lg::note, "  * system wavelength: {:.17g}", sim_params_.system_wavelength);
        lg::println(lg::note, "  * enable path loss:  {}", sim_params_.enable_path_loss);
        lg::println(lg::note, "  * n polar:           {}", sim_params_.n_polar);
        lg::println(lg::note, "  * n azimuth:         {}", sim_params_.n_azimuth);
        lg::println(lg::note, "  * n linear1:         {}", sim_params_.n_linear1);
        lg::println(lg::note, "  * n linear2:         {}", sim_params_.n_linear2);
        lg::println(lg::note, "  * xtol rel:          {:.17g}", sim_params_.xtol_rel);
        lg::println(lg::note, "  * ftol rel:          {:.17g}", sim_params_.ftol_rel);
    }

    void Setup::export_to_three(std::filesystem::path const& path_objects) const
    {
        auto const& system_wavelength = sim_params_.system_wavelength;

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
        for (auto const& ant : antennas_)
        {
            ant.visit(
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
                });
        }

        for (auto const& geo : geometries_) container.add(three::export_geometry(geo));
        container.export_to_javascript(path_objects);
        lg::println("Exported objects to {}", path_objects.string());
    }

    void Setup::run_tasks(AppParams const& params) const
    {
        bool force_recomputation = params.force_recomputation;
        if (timestamp_ == 0) force_recomputation = true;

        std::filesystem::current_path(path_cwd_);

        for (auto const& task : tasks_)
        {
            auto const id = setup::task::get_id(task);

            auto const& output_path = task::get_output_path(task);
            bool const result_found = std::filesystem::is_regular_file(output_path) and std::filesystem::file_size(output_path) > 0;
            auto const timestamp_result = result_found ? timeutil::get_of_file(output_path) : 0;
            bool const up_to_date = timestamp_result > timestamp_;
            if (up_to_date and not force_recomputation)
            {
                lg::println("Skipping computation: Output file (modified {}) is newer than setup file (modified {}).",
                    timeutil::format(timestamp_result),
                    timeutil::format(timestamp_));
                continue;
            }

            if (not params.quiet_mode)
            {
                if (up_to_date)
                    lg::println("Next task (forced):");
                else
                    lg::println("Next task:");
                lg::println("  * name:         {}", setup::task::get_name(task));
                lg::println("  * id:           {}", id);
                lg::println("  * path output:  {}", output_path.string());
            }

            task.visit(
                [&](auto const& t)
                {
                    using TaskType = std::decay_t<decltype(t)>;
                    if constexpr (std::is_same_v<TaskType, task::DirectivityOverPolarAtAzimuth>) { eval::output::directivity_over_polar(t, sim_params_); }
                    else if constexpr (std::is_same_v<TaskType, task::VoltGainOverPoints>)
                    {
                        auto const tx_coeffs = t.tx_codebook.empty() //
                            ? std::vector<Complex>(antenna::size(t.tx), 1.0)
                            : get_coeffs_from_codebook(codebooks_, t.tx_codebook);

                        auto const rx_coeffs = t.rx_codebook.empty() //
                            ? std::vector<Complex>(antenna::size(t.rx), 1.0)
                            : get_coeffs_from_codebook(codebooks_, t.rx_codebook);

                        auto const field = eval::RxVoltageField(t.tx, t.rx, tx_coeffs, rx_coeffs, sim_params_, params);
                        eval::output::voltgain::points(t, field);
                    }
                    else if constexpr (std::is_same_v<TaskType, task::VoltGainOverGeometry>)
                    {
                        auto const tx_coeffs = t.tx_codebook.empty() //
                            ? std::vector<Complex>(antenna::size(t.tx), 1.0)
                            : get_coeffs_from_codebook(codebooks_, t.tx_codebook);

                        auto const rx_coeffs = t.rx_codebook.empty() //
                            ? std::vector<Complex>(antenna::size(t.rx), 1.0)
                            : get_coeffs_from_codebook(codebooks_, t.rx_codebook);

                        auto const field = eval::RxVoltageField(t.tx, t.rx, tx_coeffs, rx_coeffs, sim_params_, params);
                        eval::output::voltgain::geometry(t, field);
                    }
                    else if constexpr (std::is_same_v<TaskType, task::VoltGainOverGeometryAtWavelength>)
                    {
                        auto const tx_coeffs = t.tx_codebook.empty() //
                            ? std::vector<Complex>(antenna::size(t.tx), 1.0)
                            : get_coeffs_from_codebook(codebooks_, t.tx_codebook);

                        auto const rx_coeffs = t.rx_codebook.empty() //
                            ? std::vector<Complex>(antenna::size(t.rx), 1.0)
                            : get_coeffs_from_codebook(codebooks_, t.rx_codebook);

                        auto const field = eval::RxVoltageField(t.tx, t.rx, tx_coeffs, rx_coeffs, sim_params_, params);
                        eval::output::voltgain::geometry_at_wavelength(t, field);
                    }
                    else if constexpr (std::is_same_v<TaskType, task::VoltGainPeakAndCutoffs>)
                    {
                        auto const tx_coeffs = t.tx_codebook.empty() //
                            ? std::vector<Complex>(antenna::size(t.tx), 1.0)
                            : get_coeffs_from_codebook(codebooks_, t.tx_codebook);

                        auto const rx_coeffs = t.rx_codebook.empty() //
                            ? std::vector<Complex>(antenna::size(t.rx), 1.0)
                            : get_coeffs_from_codebook(codebooks_, t.rx_codebook);

                        auto const field = eval::RxVoltageField(t.tx, t.rx, tx_coeffs, rx_coeffs, sim_params_, params);
                        eval::output::voltgain::curve_peak_and_cutoff(t, field);
                    }
                });
        }
    }

    Context Setup::get_context() const
    {
        return {
            .codebooks = codebooks_,
            .variables = variables_,
            .references = references_,
            .antennas = antennas_,
            .geometries = geometries_,
            .sweeps = sweeps_ //
        };
    }

    Reference const& Setup::get_reference(std::string_view const id) const { return factory::find_reference_by_id(references_, id); }

    antenna::Antenna const& Setup::get_antenna(std::string const& id) const { return antenna::get(std::span(antennas_), id); }

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
        using namespace std::ranges;
        using namespace std::views;
        auto& metadata = js.at("metadata");

        name_ = factory::get_string(metadata, "setup_name");

        auto const version_str = factory::get_string(metadata, "version");
        try
        {
            std::ranges::transform(version_str | split('.'), version_.begin(), convert::int_from_string, [](auto&& r) { return std::string_view(r); });
        }
        catch (...)
        {
            std::throw_with_nested(SimulationError("Malformed version string: '{}'", version_str));
        }

        if (version_[0] != APPLICATION_VERSION[0]) //
            throw SimulationError("Incompatible setup version: setup is v{}, but application expects major version {}.x.x",
                convert::string_from_version(version_),
                APPLICATION_VERSION[0]);
        if (version_[1] != APPLICATION_VERSION[1]) //
            lg::println(lg::warning,
                "Warning: Potentially incompatible setup version: setup is v{}, but application expects version {}.{}.x",
                convert::string_from_version(version_),
                APPLICATION_VERSION[0],
                APPLICATION_VERSION[1]);

        factory::assert_empty(metadata);
        js.erase("metadata");
    }

    void Setup::extract_codebooks(ojson& js)
    {
        if (js.contains("codebooks"))
        {
            auto const& desc = js.at("codebooks");
            try
            {
                for (auto& cb_desc : desc)
                {
                    auto id = cb_desc.at("id").get<std::string>();
                    auto path = path_cwd_ / cb_desc.at("path").get<std::string>();
                    codebooks_.push_back(std::move(Codebook::from_file(id, path)));
                }
                js.erase("codebooks");
            }
            catch (...)
            {
                std::throw_with_nested(SimulationError("Failed to parse codebooks:\n{}'", desc.dump(2)));
            }
        }
    }

    void Setup::extract_sim_params(ojson& js)
    {
        if (js.contains("sim_params"))
        {
            auto& desc = js.at("sim_params");
            try
            {
                factory::try_resolve_double_expressions(desc, variables_, "system_wavelength");
                desc.get_to(sim_params_);
                js.erase("sim_params");
            }
            catch (...)
            {
                std::throw_with_nested(SimulationError("Failed to parse numerical parameters:\n{}'", desc.dump(2)));
            }

            // add the system wavelength to the variables if it not exists
            if (not variables_.contains("system_wavelength")) variables_["system_wavelength"] = sim_params_.system_wavelength;
        }
    }

    void Setup::extract_variables(ojson& js)
    {
        variables_["c0"] = c0; // speed of light in vacuum
        variables_["epsilon0"] = epsilon0; // permittivity of vacuum (free space) / electric constant
        variables_["mu0"] = mu0; // permeability of vacuum (free space) / magnetic constant
        variables_["Z0"] = Z0; // impedance of free space

        if (!js.contains("variables")) { return; }
        for (auto& variables = js.at("variables"); const auto& [raw_key, val] : variables.items())
        {
            try
            {
                auto const colon_pos = raw_key.find(':');
                auto const stripped_key = std::string_view(raw_key).substr(0, colon_pos);
                auto const type = colon_pos == std::string_view::npos ? std::string_view() : std::string_view(raw_key).substr(colon_pos + 1);
                bool is_double = type.empty() or type == "double";
                if (!is_double and type != "int") { throw SimulationError("Variable '{}' has invalid type specifier '{}'", stripped_key, type); }
                std::string key(stripped_key);

                if (variables_.contains(key)) throw SimulationError("Variable '{}' already exists", key);

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
        for (auto& antennas = js.at("antennas"); auto& desc : antennas) antennas_.push_back(factory::make_antenna(desc, variables_));
        js.erase("antennas");
    }

    void Setup::extract_geometries(ojson& js)
    {
        if (!js.contains("geometries")) { return; }
        for (auto& geometries = js.at("geometries"); auto& desc : geometries) geometries_.push_back(factory::make_geometry(desc, variables_));
        js.erase("geometries");
    }

    void Setup::extract_sweeps(ojson& js)
    {
        if (!js.contains("sweeps")) { return; }
        for (auto& sweeps = js.at("sweeps"); auto& desc : sweeps) sweeps_.push_back(factory::make_sweep(desc, variables_));
        js.erase("sweeps");
    }

    void Setup::extract_tasks(ojson& js)
    {
        if (!js.contains("tasks")) { return; }
        for (auto& tasks = js.at("tasks"); auto& desc : tasks)
        {
            auto t = task::from_json(js, get_context());
            (void)task::get_output_type(t); // assert that the task has a valid output type
            tasks_.push_back(t);
        }
        js.erase("tasks");
    }
} // namespace setup
