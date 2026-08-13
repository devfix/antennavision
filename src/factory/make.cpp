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
#include "lg.hpp"
#include "math/coords.hpp"
#include "math/functions.hpp"

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

        Radiator make_radiator(ojson& desc, VarMap const& variables)
        {
            auto const id = get_string(desc, "id");
            auto const origin_id = get_string(desc, "ref", true, true);
            auto const type = get_string(desc, "type");

            if (type == "IsotropicRadiator") return Radiator::IsotropicRadiator::create(id, origin_id);
            if (type == "HertzianDipole") return Radiator::HertzianDipole::create(id, origin_id);
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
                return {.type = Radiator::Type::CustomRadiator, .id = id, .origin_id = origin_id, .elv_spherical = std::move(effective_length)};
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
                double const t = math::nidx(k, size);
                Pos const pos = dir * (t - 0.5) * length;
                references.push_back(Reference{
                    .id = std::format("{}:ref:{}", id, k),
                    .origin_id = origin_id,
                    .pos = pos,
                    .rot = rot //
                });

                // We make a copy of the "backup" description and adapt it for the current element of the ULA
                ojson ula_element_desc = prototype_desc;
                ula_element_desc["id"] = std::format("{}:rad:{}", id, k);
                ula_element_desc["ref"] = references.back().id;

                // call the make function recursively and append the Radiators to array_radiators
                elements.push_back(make_radiator(ula_element_desc, variables));
            }

            return UniformLinearArray{{
                .type = RadiatorArrayType::UniformLinearArray,
                .id = id,
                .origin_id = origin_id,
                .references = std::move(references),
                .elements = std::move(elements) //
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
                    double const tx = math::nidx(x, size_x);
                    double const ty = math::nidx(y, size_y);
                    Pos const pos = Pos(1.0, 0.0, 0.0) * (tx - 0.5) * length_x + Pos(0.0, 1.0, 0.0) * (ty - 0.5) * length_y;
                    references.push_back(Reference{
                        .id = std::format("{}:ref:{}:{}", id, x, y),
                        .origin_id = origin_id,
                        .pos = pos,
                        .rot = rot //
                    });

                    // We make a copy of the "backup" description and adapt it for the current element of the ULA
                    ojson ula_element_desc = prototype_desc;
                    ula_element_desc["id"] = std::format("{}:rad:{}:{}", id, x, y);
                    ula_element_desc["ref"] = references.back().id;

                    // call the make function recursively and append the Radiators to array_radiators
                    elements.push_back(make_radiator(ula_element_desc, variables));
                }
            }

            return UniformPlanarArray{
                {
                    .type = RadiatorArrayType::UniformPlanarArray,
                    .id = id,
                    .origin_id = origin_id,
                    .references = std::move(references),
                    .elements = std::move(elements) //
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
            try_resolve_double_expressions(desc, variables, "values");
            try_resolve_double_expressions(desc, variables, "begin");
            try_resolve_double_expressions(desc, variables, "end");
            try_resolve_double_expressions(desc, variables, "base");
            auto sweep = desc.get<sweep::Sweep>();
            assert_valid_id(sweep::get_id(sweep));
            return sweep;
        }
        catch (...)
        {
            std::throw_with_nested(SimulationError("Failed to parse sweep:\n{}", desc.dump(2)));
        }
    }

    setup::task::Task make_task(ojson& desc, VarMap const& variables)
    {
        try
        {
            try_resolve_int_expressions(desc, variables, "n_dim1");
            try_resolve_int_expressions(desc, variables, "n_dim2");
            try_resolve_double_expressions(desc, variables, "points");
            try_resolve_double_expressions(desc, variables, "ratio");
            try_resolve_double_expressions(desc, variables, "wavelength");
            return desc.get<setup::task::Task>(); // no id check here since the user doesn't choose the id by himself
        }
        catch (...)
        {
            std::throw_with_nested(SimulationError("Failed to parse task:\n{}", desc.dump(2)));
        }
    }
} // namespace factory
