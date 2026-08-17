//
// Created by Tristan Krause on 2026-06-18.
//

#include "factory/make.hpp"
#include <locale>
#include <print>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include "NumCpp/Functions/var.hpp"
#include "factory/find.hpp"
#include "factory/get.hpp"
#include "factory/parse.hpp"
#include "math/coords.hpp"

namespace factory
{
    using reference::Reference;
    using components::Radiator;
    using components::RadiatorArray;
    using components::Antenna;

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

        Radiator::Descriptor get_radiator_desc(ojson& desc, VarMap const& variables)
        {
            std::string id = desc.contains("id") ? desc.at("id").get<std::string>() : "";
            std::string origin_id = desc.contains("ref") ? desc.at("ref").get<std::string>() : "";
            auto const type_str = get_string(desc, "type");
            auto type = magic_enum::enum_cast<Radiator::Type>(type_str);
            if (not type) throw SimulationError("Unknown radiator type '{}'", type_str);
            std::optional<double> dipole_length;
            if (desc.contains("dipole_length"))
            {
                try_resolve_double_expressions(desc, variables, "dipole_length");
                dipole_length = {get_double(desc, "dipole_length", variables)};
            }
            std::optional<Radiator::elv_spherical_t> elv_spherical;
            if (desc.contains(""))
            {
                auto const effective_length_defs = get_string_vec3(desc, "effective_length");
                std::array<std::function<Complex(double, double, double)>, 3> effective_length_parts;
                std::ranges::transform(effective_length_defs, effective_length_parts.begin(), parse_polar_azimuth_function);
                elv_spherical = {[effective_length_parts](double const polar, double const azimuth, double const wavelength) -> ComplexArray
                    {
                        return {effective_length_parts[0](polar, azimuth, wavelength),
                            effective_length_parts[1](polar, azimuth, wavelength),
                            effective_length_parts[2](polar, azimuth, wavelength)};
                    }};
            }
            return Radiator::Descriptor{
                .type = type.value(),
                .id = std::move(id),
                .origin_id = std::move(origin_id),
                .dipole_length = dipole_length,
                .elv_spherical = std::move(elv_spherical) //
            };
        }

        components::Antenna make_ula(ojson& js, VarMap const& variables)
        {
            try_resolve_double_expressions(js, variables, "spacing");
            try_resolve_int_expressions(js, variables, "size");
            try_resolve_double_expressions(js, variables, "rot");

            auto const id = get_string(js, "id");
            auto const origin_id = get_string(js, "ref", true, true);
            auto const type = get_string(js, "type");
            auto const spacing = get_double(js, "spacing", variables);
            auto const size = get_uint(js, "size", variables);
            Quaternion rot;
            if (js.contains("rot")) { js.at("rot").get_to(rot); }
            auto prototype_desc = get_radiator_desc(js.at("radiator"), variables);
            js.erase("radiator");

            auto desc = components::RadiatorArray::Desciptor{
                .type = components::RadiatorArray::Type::UniformLinearArray,
                .id = id,
                .origin_id = origin_id,
                .rot = rot,
                .prototype_desc = std::move(prototype_desc),
                .parameters = components::RadiatorArray::UniformLinearParameters{.spacing = spacing, .size = size} //
            };

            return std::move(components::RadiatorArray::create(desc));
        }

        components::Antenna make_upa(ojson& js, VarMap const& variables)
        {
            try_resolve_double_expressions(js, variables, "spacing_x");
            try_resolve_double_expressions(js, variables, "spacing_y");
            try_resolve_int_expressions(js, variables, "size_x");
            try_resolve_int_expressions(js, variables, "size_y");
            try_resolve_double_expressions(js, variables, "rot");

            auto const id = get_string(js, "id");
            auto const origin_id = get_string(js, "ref", true, true);
            auto const type = get_string(js, "type");
            auto const spacing_x = get_double(js, "spacing_x", variables);
            auto const spacing_y = get_double(js, "spacing_y", variables);
            auto const size_x = get_uint(js, "size_x", variables);
            auto const size_y = get_uint(js, "size_y", variables);
            Quaternion rot;
            if (js.contains("rot")) { js.at("rot").get_to(rot); }
            auto prototype_desc = get_radiator_desc(js.at("radiator"), variables);
            js.erase("radiator");

            auto desc = components::RadiatorArray::Desciptor{
                .type = components::RadiatorArray::Type::UniformPlanarArray,
                .id = id,
                .origin_id = origin_id,
                .rot = rot,
                .prototype_desc = std::move(prototype_desc),
                .parameters =
                    components::RadiatorArray::UniformPlanarParameters{
                        .spacing_x = spacing_x,
                        .spacing_y = spacing_y,
                        .size_x = size_x,
                        .size_y = size_y //
                    } //
            };

            return std::move(components::RadiatorArray::create(desc));
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

    components::Antenna make_antenna(ojson& desc, VarMap const& variables)
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
            return Radiator::create(get_radiator_desc(desc, variables));
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
} // namespace factory
