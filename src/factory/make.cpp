//
// Created by Tristan Krause on 2026-06-18.
//

#include "factory/make.hpp"
#include <ansi_color.hpp>
#include <locale>
#include <nlohmann/json.hpp>
#include <print>
#include "NumCpp/Functions/var.hpp"
#include "factory/find.hpp"
#include "factory/get.hpp"
#include "factory/parse.hpp"

using ansi_color::fg4;
using ansi_color::reset;

namespace factory
{
    using reference::Reference;

    namespace
    {
        bool assert_valid_id(std::string_view id)
        {
            for (char c : id)
            {
                if (!std::isalnum(c, std::locale::classic()) and c != '_' and c != '-')
                {
                    throw SimulationError("Invalid id '{}', bad character: '{}'", id, c);
                }
            }
            return true;
        }

        std::vector<std::complex<double>>
        load_upa_codebook(const std::filesystem::path& filepath, const std::string& ratio_key, const std::string& row, const std::string& col)
        {
            // 1. Open and parse the JSON file
            std::ifstream file(filepath);
            if (!file.is_open()) { throw std::runtime_error("Failed to open codebook file: " + filepath.string()); }

            nlohmann::json root;
            file >> root;

            std::vector<Complex> coeffs;
            auto& arr = root.at(ratio_key).at(row).at(col);
            for (std::size_t k = 0; k < arr.size(); k++) { coeffs.push_back(math::complex_from_polar(arr[k][0], arr[k][1])); }
            return coeffs;

            if (!root.contains(ratio_key)) { throw std::invalid_argument("Distance ratio '" + ratio_key + "' not found in the codebook."); }

            const auto& ratio_data = root[ratio_key];

            // 2. Query the sizes of the beam grid (N_grid and M_grid) dynamically
            int n_grid = 0;
            for (const auto& [key, val] : ratio_data.items()) { n_grid = std::max(n_grid, std::stoi(key) + 1); }

            if (n_grid == 0) { throw std::runtime_error("Empty grid configuration for ratio: " + ratio_key); }

            int m_grid = 0;
            const auto& first_row = ratio_data["0"];
            for (const auto& [key, val] : first_row.items()) { m_grid = std::max(m_grid, std::stoi(key) + 1); }

            // 3. Determine the number of physical antenna elements (N * M)
            const auto& sample_beam = first_row["0"];
            const size_t num_elements = sample_beam.size();
            const size_t num_beams = static_cast<size_t>(n_grid) * static_cast<size_t>(m_grid);

            // 4. Initialize NumCpp 2D array of size (num_beams, num_elements)
            auto codebook_matrix = nc::NdArray<std::complex<double>>(num_beams, num_elements);

            // 5. Populate the matrix
            for (int r = 0; r < n_grid; ++r)
            {
                const auto& row_data = ratio_data[std::to_string(r)];

                for (int c = 0; c < m_grid; ++c)
                {
                    const auto& polar_weights = row_data[std::to_string(c)];
                    const size_t beam_idx = r * m_grid + c; // Flattened beam index

                    for (size_t elem_idx = 0; elem_idx < num_elements; ++elem_idx)
                    {
                        // Extract magnitude and phase angle [abs, angle]
                        const double magnitude = polar_weights[elem_idx][0].get<double>();
                        const double angle = polar_weights[elem_idx][1].get<double>();

                        // Reconstruct complex weight using Euler's relation: mag * e^(i * angle)
                        codebook_matrix(beam_idx, elem_idx) = std::polar(magnitude, angle);
                    }
                }
            }
        }

        Radiator make_radiator(ojson& desc, VarMap const& variables)
        {
            auto const id = get_string(desc, "id");
            auto const origin_id = get_string(desc, "ref", true, true);
            auto const type = get_string(desc, "type");
            // std::println("{}Creating radiator [id: '{}', origin: '{}', type: '{}']{}", fg4::bright_black, id, origin_id, type, reset);
            if (type == "HertzianDipole") { return Radiator::HertzianDipole::create(id, origin_id); }
            if (type == "StandingWaveDipole")
            {
                try_resolve_double_expressions(desc, variables, "dipole_length");
                auto const dipole_length = get_double(desc, "dipole_length", variables);
                return Radiator::StandingWaveDipole::create(id, origin_id, dipole_length);
            }
            if (type == "CustomRadiator")
            {
                auto const effective_length_defs = get_string_vec3(desc, "effective_length");
                std::array<std::function<Complex(double, double, double)>, 3> effective_length_parts;
                std::ranges::transform(effective_length_defs, effective_length_parts.begin(), parse_polar_azimuth_function);
                auto effective_length = [effective_length_parts](double const polar, double const azimuth, double const wavelength) -> ComplexArray
                {
                    return {effective_length_parts[0](polar, azimuth, wavelength),
                        effective_length_parts[1](polar, azimuth, wavelength),
                        effective_length_parts[2](polar, azimuth, wavelength)};
                };
                return {id, origin_id, std::move(effective_length)};
            }
            throw SimulationError("Unknown radiator type '{}'", type);
        }

        antenna::Antenna make_ula(ojson& desc, VarMap const& variables)
        {
            try_resolve_double_expressions(desc, variables, "spacing");
            try_resolve_int_expressions(desc, variables, "size");
            try_resolve_double_expressions(desc, variables, "rot");

            auto const id = get_string(desc, "id");
            auto const origin_id = get_string(desc, "ref", true, true);
            auto const type = get_string(desc, "type");
            auto const spacing = get_double(desc, "spacing", variables);
            auto const size = get_uint(desc, "size", variables);
            Quaternion rot;
            if (desc.contains("rot")) { desc.at("rot").get_to(rot); }
            auto const prototype_desc = desc.at("radiator");
            desc.erase("radiator");

            Pos constexpr dir(1.0, 0.0, 0.0);
            double const length = spacing * (size - 1);
            std::vector<Radiator> elements;
            elements.reserve(size);
            std::vector<Reference> references;
            references.reserve(size);
            for (std::size_t k = 0; k < size; k++)
            {
                double const t = static_cast<double>(k) / static_cast<double>(size - 1);
                Pos const pos = dir * (t - 0.5) * length;
                references.push_back(Reference{
                    .id = std::format("{}", k),
                    .origin_id = origin_id,
                    .pos = pos,
                    .rot = rot //
                });

                // We make a copy of the "backup" description and adapt it for the current element of the ULA
                ojson ula_element_desc = prototype_desc;
                ula_element_desc["id"] = std::format("{}", k);
                ula_element_desc["ref"] = references.back().id;

                // call the make function recursively and append the Radiators to array_radiators
                elements.push_back(make_radiator(ula_element_desc, variables));
            }

            std::vector<Complex> coeffs(size, 1.0); // default values
            if (desc.contains("codebook"))
            {
                throw SimulationError("codebook not implemented for ULA");
                // auto& codebook = desc.at("codebook");
                // auto const codebook_path = codebook.at("file").get<std::string>();
                // auto const codebook_ratio = codebook.at("ratio").get<std::string>();
                // coeffs = load_upa_codebook(codebook_path, codebook_ratio);
                desc.erase("codebook");
            }
            assert(size == coeffs.size());
            return UniformLinearArray{{
                .id = id,
                .origin_id = origin_id,
                .references = references,
                .elements = std::move(elements),
                .coefficients = std::move(coeffs) //
            }};
        }

        antenna::Antenna make_upa(ojson& desc, VarMap const& variables)
        {
            try_resolve_double_expressions(desc, variables, "spacing_x");
            try_resolve_double_expressions(desc, variables, "spacing_y");
            try_resolve_int_expressions(desc, variables, "size_x");
            try_resolve_int_expressions(desc, variables, "size_y");
            try_resolve_double_expressions(desc, variables, "rot");

            auto const id = get_string(desc, "id");
            auto const origin_id = get_string(desc, "ref", true, true);
            auto const type = get_string(desc, "type");
            auto const spacing_x = get_double(desc, "spacing_x", variables);
            auto const spacing_y = get_double(desc, "spacing_y", variables);
            auto const size_x = get_uint(desc, "size_x", variables);
            auto const size_y = get_uint(desc, "size_y", variables);
            Quaternion rot;
            if (desc.contains("rot")) { desc.at("rot").get_to(rot); }
            auto const prototype_desc = desc.at("radiator");
            desc.erase("radiator");

            double const length_x = spacing_x * (size_x - 1);
            double const length_y = spacing_y * (size_y - 1);
            std::vector<Radiator> elements;
            elements.reserve(size_x * size_y);
            std::vector<Reference> references;
            references.reserve(size_x * size_y);
            for (std::decay_t<decltype(size_y)> y = 0; y < size_y; y++)
            {
                for (std::decay_t<decltype(size_x)> x = 0; x < size_x; x++)
                {
                    double const fx = static_cast<double>(x) / static_cast<double>(size_x - 1);
                    double const fy = static_cast<double>(y) / static_cast<double>(size_y - 1);
                    Pos const pos = Pos(1.0, 0.0, 0.0) * (fx - 0.5) * length_x + Pos(0.0, 1.0, 0.0) * (fy - 0.5) * length_y;
                    references.push_back(Reference{
                        .id = std::format("ref:{}:{}", x, y),
                        .origin_id = origin_id,
                        .pos = pos,
                        .rot = rot //
                    });

                    // We make a copy of the "backup" description and adapt it for the current element of the ULA
                    ojson ula_element_desc = prototype_desc;
                    ula_element_desc["id"] = std::format("{}:radiator:{}:{}", id, x, y);
                    ula_element_desc["ref"] = references.back().id;

                    // call the make function recursively and append the Radiators to array_radiators
                    elements.push_back(make_radiator(ula_element_desc, variables));
                }
            }

            std::vector<Complex> coeffs(size_x * size_y, 1.0); // default values
            if (desc.contains("codebook"))
            {
                auto& codebook = desc.at("codebook");
                if (codebook.empty()) { std::println("{}Warning: empty codebook for UPA{}", ansi_color::fg4::bright_yellow, ansi_color::reset); }
                else
                {
                    auto const codebook_path = codebook.at("file").get<std::string>();
                    auto const codebook_ratio = codebook.at("ratio").get<std::string>();
                    auto const codebook_row = codebook.at("row").get<std::string>();
                    auto const codebook_col = codebook.at("col").get<std::string>();
                    coeffs = load_upa_codebook(codebook_path, codebook_ratio, codebook_row, codebook_col);
                }
                desc.erase("codebook");
            }
            assert(size_x * size_y == coeffs.size());
            return UniformPlanarArray{
                {
                    .id = id,
                    .origin_id = origin_id,
                    .references = std::move(references),
                    .elements = std::move(elements),
                    .coefficients = std::move(coeffs) //
                },
                size_x,
                size_y //
            };
        }
    } // namespace

    Reference make_reference(ojson& desc, VarMap const& variables)
    {
        try
        {
            try_resolve_double_expressions(desc, variables, "pos");
            try_resolve_double_expressions(desc, variables, "rot");
            Reference const ref = desc.get<Reference>();
            assert_valid_id(ref.id);
            return ref;
        }
        catch (...)
        {
            std::throw_with_nested(SimulationError("Failed to parse reference:\n{}", desc.dump(2)));
        }
    }

    antenna::Antenna make_antenna(ojson& desc, VarMap const& variables)
    {
        try
        {
            // we only read the id, type and ref (origin id) from the description but do not delete them yet
            auto const id = get_string(desc, "id", false);
            assert_valid_id(id);
            auto const type = get_string(desc, "type", false);
            auto const origin_id = get_string(desc, "ref", false, true);
            std::println("{}Creating antenna [id: '{}', origin: '{}', type: '{}']{}", fg4::bright_black, id, origin_id, type, reset);

            // depending on the type make a ULA, UPA or single radiator as the antenna
            if (type == "ULA") { return make_ula(desc, variables); }
            if (type == "UPA") { return make_upa(desc, variables); }
            return make_radiator(desc, variables);
        }
        catch (...)
        {
            std::throw_with_nested(SimulationError("Failed to parse antenna:\n{}", desc.dump(2)));
        }
    }

    geometry::Geometry make_geometry(ojson& desc, VarMap const& variables)
    {
        try
        {
            try_resolve_double_expressions(desc, variables, "pos_begin");
            try_resolve_double_expressions(desc, variables, "pos_end");
            try_resolve_double_expressions(desc, variables, "center");
            try_resolve_double_expressions(desc, variables, "normal");
            try_resolve_double_expressions(desc, variables, "e1");
            try_resolve_double_expressions(desc, variables, "e2");
            try_resolve_double_expressions(desc, variables, "radius");
            try_resolve_double_expressions(desc, variables, "angle_span");
            try_resolve_double_expressions(desc, variables, "width");
            try_resolve_double_expressions(desc, variables, "height");
            try_resolve_double_expressions(desc, variables, "polar_span");
            try_resolve_double_expressions(desc, variables, "azimuth_span");
            auto geo = desc.get<geometry::Geometry>();
            assert_valid_id(geometry::get_id(geo));
            return geo;
        }
        catch (...)
        {
            std::throw_with_nested(SimulationError("Failed to parse geometry:\n{}", desc.dump(2)));
        }
    }

    sweep::Sweep make_sweep(ojson& desc, VarMap const& variables)
    {
        try
        {
            if (!desc.contains("type")) throw SimulationError("Missing sweep type");
            if (desc.at("type").type() != nlohmann::json::value_t::string)
                throw SimulationError("Sweep attribute type must be string, but is {}", desc.at("type").type_name());
            auto const type = desc.at("type").get<std::string>();
            desc.erase("type");

            sweep::Sweep sweep;
            if (type == "ListSweep")
            {
                try_resolve_double_expressions(desc, variables, "values");
                sweep = desc.get<sweep::ListSweep>();
            }
            else if (type == "LinearSweep")
            {
                try_resolve_double_expressions(desc, variables, "begin");
                try_resolve_double_expressions(desc, variables, "end");
                sweep = desc.get<sweep::LinearSweep>();
            }
            else if (type == "LogSweep")
            {
                try_resolve_double_expressions(desc, variables, "begin");
                try_resolve_double_expressions(desc, variables, "end");
                try_resolve_double_expressions(desc, variables, "base");
                sweep = desc.get<sweep::LogSweep>();
            }
            else
                throw SimulationError("Unknown sweep type '{}'", type);

            assert_valid_id(sweep::get_id(sweep));
            return sweep;
        }
        catch (...)
        {
            std::throw_with_nested(SimulationError("Failed to parse sweep:\n{}", desc.dump(2)));
        }
    }
} // namespace factory
